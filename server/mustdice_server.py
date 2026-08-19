#!/usr/bin/env python3
# MustDice dedicated match server (stdlib asyncio only).
from __future__ import annotations

import asyncio
import random
import time
from collections import deque

HOST = "0.0.0.0"
PORT = 7777
MAX_PLAYERS = 4
BETS_PER_ROUND = 3
TOTAL_ROUNDS = 5
BET_TIMEOUT_SEC = 18.0
RESOLVE_WAIT_SEC = 8.0
ROUND_BREAK_SEC = 4.0
POINTS = (400, 300, 200, 100)


def score_pinpoint(pick_sum: int, rolled_sum: int) -> int:
    pick = min(12, max(2, pick_sum))
    rolled = min(12, max(2, rolled_sum))
    dist = abs(pick - 7)
    mult = 1.0 + 0.1 * dist
    factor = mult - 0.1 * abs(pick - rolled)
    return int(100.0 * factor + 0.5)


def score_odd_even(pick_odd: bool, rolled_sum: int) -> int:
    hit = ((rolled_sum % 2) != 0) == pick_odd
    return 120 if hit else 60


def roll_2d6():
    a = random.randint(1, 6)
    b = random.randint(1, 6)
    return a, b, a + b


class Player:
    def __init__(self, writer: asyncio.StreamWriter):
        self.writer = writer
        self.name = ""
        self.player_id = -1
        self.connected = True
        self.ready = False
        self.submitted = False
        self.kind = "O"
        self.value = 1
        self.auto = False
        self.round_score = 0
        self.total_round_score = 0
        self.match_point_x100 = 0
        self.first_place_count = 0
        self.last_rank = 0
        self.last_score = 0
        self.die0 = 0
        self.die1 = 0
        self.queue = 0
        self.bound = None


class Match:
    def __init__(self, players: list):
        self.players = players
        for i, p in enumerate(players):
            p.player_id = i
            p.round_score = 0
            p.total_round_score = 0
            p.match_point_x100 = 0
            p.first_place_count = 0
            p.ready = False
            p.submitted = False
        self.round = 1
        self.bet = 1
        self.deadline = 0.0
        self.phase = "betting"


class ServerState:
    def __init__(self):
        self.open_lobby = []
        self.wait_lobby = []
        self.overflow = deque()
        self.match = None
        self.lock = asyncio.Lock()


STATE = ServerState()


async def send_line(player: Player, line: str) -> None:
    if not player.connected:
        return
    try:
        player.writer.write((line + "\n").encode("utf-8"))
        await player.writer.drain()
    except Exception:
        player.connected = False


async def broadcast_match(match: Match, line: str) -> None:
    for p in match.players:
        await send_line(p, line)


def lobby_names(players: list[Player]) -> str:
    return " ".join(p.name if p.name else "-" for p in players)


def ready_mask(players: list[Player]) -> int:
    mask = 0
    for i, p in enumerate(players):
        if p.ready:
            mask |= 1 << i
    return mask


def submitted_mask(players: list[Player]) -> int:
    mask = 0
    for i, p in enumerate(players):
        if p.submitted:
            mask |= 1 << i
    return mask


async def send_lobby(players: list[Player], waiting_behind: int, queue_flag: int) -> None:
    count = len(players)
    mask = ready_mask(players)
    line = f"LOBBY {count} {mask} {waiting_behind} {lobby_names(players)}"
    for p in players:
        p.queue = queue_flag
        await send_line(p, line)


async def fill_wait_from_overflow() -> None:
    while len(STATE.wait_lobby) < MAX_PLAYERS and STATE.overflow:
        nxt = STATE.overflow.popleft()
        if nxt.connected:
            nxt.ready = False
            STATE.wait_lobby.append(nxt)
            await send_line(nxt, f"WELCOME {nxt.player_id} 1")


def all_ready(players: list[Player]) -> bool:
    return 2 <= len(players) <= MAX_PLAYERS and all(p.ready for p in players)


def add_round_points(players: list[Player]) -> None:
    n = len(players)
    slots = list(POINTS[:n])
    order = sorted(range(n), key=lambda i: players[i].round_score, reverse=True)
    i = 0
    while i < n:
        j = i + 1
        score = players[order[i]].round_score
        while j < n and players[order[j]].round_score == score:
            j += 1
        group = order[i:j]
        share = sum(slots[i:j]) // len(group)
        rank = i + 1
        for idx in group:
            players[idx].match_point_x100 += share
            players[idx].last_rank = rank
            if rank == 1:
                players[idx].first_place_count += 1
        i = j


def final_order(players: list[Player]) -> list[int]:
    n = len(players)

    def key(i: int):
        p = players[i]
        return (-p.match_point_x100, -p.first_place_count, -p.total_round_score, i)

    return sorted(range(n), key=key)


async def send_resolve(match: Match) -> None:
    parts = [f"RESOLVE {len(match.players)}"]
    for p in match.players:
        kind = p.kind
        val = p.value
        auto = 1 if p.auto else 0
        parts.append(
            f"{p.player_id} {kind} {val} {p.die0} {p.die1} {p.last_score} {p.round_score} {auto}"
        )
    await broadcast_match(match, " ".join(parts))


async def send_round_end(match: Match) -> None:
    parts = [f"ROUND_END {len(match.players)} {match.round}"]
    for p in match.players:
        parts.append(
            f"{p.player_id} {p.last_rank} {p.match_point_x100} {p.first_place_count} {p.total_round_score} {p.round_score}"
        )
    await broadcast_match(match, " ".join(parts))


async def send_match_end(match: Match) -> None:
    order = final_order(match.players)
    ranks = [0] * len(match.players)
    for place, idx in enumerate(order, start=1):
        ranks[idx] = place
        # ties on final key
    for i in range(len(order)):
        a = order[i]
        pa = match.players[a]
        for j in range(i + 1, len(order)):
            b = order[j]
            pb = match.players[b]
            if (
                pa.match_point_x100 == pb.match_point_x100
                and pa.first_place_count == pb.first_place_count
                and pa.total_round_score == pb.total_round_score
            ):
                ranks[b] = ranks[a]
            else:
                break
    parts = [f"MATCH_END {len(match.players)}"]
    for p in match.players:
        parts.append(
            f"{p.player_id} {ranks[p.player_id]} {p.match_point_x100} {p.first_place_count} {p.total_round_score}"
        )
    await broadcast_match(match, " ".join(parts))


def remain_sec(deadline: float) -> int:
    left = int(deadline - time.time())
    if left < 0:
        return 0
    return left


async def open_bet(match: Match) -> None:
    match.phase = "betting"
    match.deadline = time.time() + BET_TIMEOUT_SEC
    for p in match.players:
        p.submitted = False
        p.auto = False
    await broadcast_match(
        match,
        f"BET_OPEN {match.round} {match.bet} {remain_sec(match.deadline)}",
    )


async def resolve_job(match: Match) -> None:
    async with STATE.lock:
        if STATE.match is not match or match.phase != "betting":
            return
        match.phase = "resolving"
        for p in match.players:
            if not p.submitted:
                p.kind = "O"
                p.value = random.randint(0, 1)
                p.auto = True
                p.submitted = True
            d0, d1, total = roll_2d6()
            p.die0, p.die1 = d0, d1
            if p.kind == "P":
                p.last_score = score_pinpoint(p.value, total)
            else:
                p.last_score = score_odd_even(bool(p.value), total)
            p.round_score += p.last_score
        await send_resolve(match)
    await asyncio.sleep(RESOLVE_WAIT_SEC)
    async with STATE.lock:
        if STATE.match is not match:
            return
        match.bet += 1
        if match.bet > BETS_PER_ROUND:
            add_round_points(match.players)
            for p in match.players:
                p.total_round_score += p.round_score
            await send_round_end(match)
            if match.round >= TOTAL_ROUNDS:
                await send_match_end(match)
                await finish_match()
                return
        else:
            await open_bet(match)
            return
    await asyncio.sleep(ROUND_BREAK_SEC)
    async with STATE.lock:
        if STATE.match is not match:
            return
        match.round += 1
        match.bet = 1
        for p in match.players:
            p.round_score = 0
        await open_bet(match)


async def finish_match() -> None:
    match = STATE.match
    STATE.match = None
    if match:
        for p in match.players:
            p.player_id = -1
            p.ready = False
    STATE.open_lobby = [p for p in STATE.wait_lobby if p.connected]
    STATE.wait_lobby = []
    await fill_wait_from_overflow()
    extra = []
    while len(STATE.open_lobby) < MAX_PLAYERS and STATE.overflow:
        nxt = STATE.overflow.popleft()
        if nxt.connected:
            extra.append(nxt)
    STATE.open_lobby.extend(extra)
    for i, p in enumerate(STATE.open_lobby):
        p.player_id = i
        p.queue = 0
        await send_line(p, f"WELCOME {p.player_id} 0")
    await send_lobby(STATE.open_lobby, len(STATE.overflow), 0)
    if all_ready(STATE.open_lobby):
        await start_match_from_open()


async def start_match_from_open() -> None:
    players = list(STATE.open_lobby)
    STATE.open_lobby = []
    STATE.match = Match(players)
    await broadcast_match(STATE.match, f"MATCH_START {len(players)}")
    await open_bet(STATE.match)


async def try_start_match() -> None:
    if STATE.match is not None:
        return
    if all_ready(STATE.open_lobby):
        await start_match_from_open()


def find_reconnect(name: str) -> Player | None:
    if STATE.match is None:
        return None
    for p in STATE.match.players:
        if p.name == name and not p.connected:
            return p
    return None


def name_taken_connected(name: str) -> bool:
    pools = list(STATE.open_lobby) + list(STATE.wait_lobby) + list(STATE.overflow)
    if STATE.match:
        pools += STATE.match.players
    for p in pools:
        if p.connected and p.name == name:
            return True
    return False


async def place_new_player(player: Player) -> None:
    if STATE.match is not None:
        if len(STATE.wait_lobby) < MAX_PLAYERS:
            player.player_id = len(STATE.wait_lobby)
            player.queue = 1
            STATE.wait_lobby.append(player)
            await send_line(player, f"WELCOME {player.player_id} 1")
            await send_lobby(STATE.wait_lobby, len(STATE.overflow), 1)
        else:
            player.queue = 1
            player.player_id = -1
            STATE.overflow.append(player)
            await send_line(player, "WELCOME -1 1")
            await send_line(player, f"LOBBY 0 0 {len(STATE.overflow)} ")
        return
    if len(STATE.open_lobby) < MAX_PLAYERS:
        player.player_id = len(STATE.open_lobby)
        player.queue = 0
        STATE.open_lobby.append(player)
        await send_line(player, f"WELCOME {player.player_id} 0")
        await send_lobby(STATE.open_lobby, len(STATE.overflow) + len(STATE.wait_lobby), 0)
    else:
        player.queue = 1
        STATE.overflow.append(player)
        await send_line(player, "WELCOME -1 1")
        await send_line(player, f"LOBBY 0 0 {len(STATE.overflow)} ")


async def drop_player(player: Player) -> None:
    player.connected = False
    if STATE.match and player in STATE.match.players:
        return
    if player in STATE.open_lobby:
        STATE.open_lobby.remove(player)
        for i, p in enumerate(STATE.open_lobby):
            p.player_id = i
        await send_lobby(STATE.open_lobby, len(STATE.overflow), 0)
        return
    if player in STATE.wait_lobby:
        STATE.wait_lobby.remove(player)
        for i, p in enumerate(STATE.wait_lobby):
            p.player_id = i
        await fill_wait_from_overflow()
        await send_lobby(STATE.wait_lobby, len(STATE.overflow), 1)
        return
    if player in STATE.overflow:
        STATE.overflow = deque([p for p in STATE.overflow if p is not player])


async def send_snap(player: Player) -> None:
    match = STATE.match
    if match is None or player not in match.players:
        return
    names = lobby_names(match.players)
    await send_line(
        player,
        "SNAP "
        f"{player.player_id} {match.round} {match.bet} {remain_sec(match.deadline)} "
        f"{submitted_mask(match.players)} {len(match.players)} "
        f"{player.round_score} {player.match_point_x100} {names}",
    )


async def handle_line(player: Player, line: str) -> None:
    parts = line.strip().split()
    if not parts:
        return
    cmd = parts[0].upper()
    if cmd == "HELLO":
        if len(parts) < 2:
            await send_line(player, "ERR NAME")
            return
        name = parts[1]
        if (not name) or (" " in name) or (len(name) > 16):
            await send_line(player, "ERR NAME")
            return
        recon = find_reconnect(name)
        if recon is not None:
            recon.writer = player.writer
            recon.connected = True
            player.bound = recon
            await send_line(recon, f"WELCOME {recon.player_id} 0")
            await send_snap(recon)
            return
        if name_taken_connected(name):
            await send_line(player, "ERR NAME")
            return
        player.name = name
        await place_new_player(player)
        return
    if not player.name:
        await send_line(player, "ERR NAME")
        return
    if cmd == "READY":
        if STATE.match and player in STATE.match.players:
            return
        player.ready = True
        if player in STATE.open_lobby:
            await send_lobby(STATE.open_lobby, len(STATE.overflow), 0)
            await try_start_match()
        elif player in STATE.wait_lobby:
            await send_lobby(STATE.wait_lobby, len(STATE.overflow), 1)
        return
    if cmd == "BET":
        if STATE.match is None or player not in STATE.match.players:
            return
        if STATE.match.phase != "betting" or player.submitted:
            return
        if len(parts) < 3:
            await send_line(player, "ERR BAD")
            return
        kind = parts[1].upper()
        try:
            value = int(parts[2])
        except ValueError:
            await send_line(player, "ERR BAD")
            return
        if kind == "P":
            if value < 2 or value > 12:
                await send_line(player, "ERR BAD")
                return
            player.kind = "P"
            player.value = value
        elif kind == "O":
            if value not in (0, 1):
                await send_line(player, "ERR BAD")
                return
            player.kind = "O"
            player.value = value
        else:
            await send_line(player, "ERR BAD")
            return
        player.submitted = True
        player.auto = False
        await broadcast_match(
            STATE.match, f"BET_WAIT {submitted_mask(STATE.match.players)}"
        )
        if all(p.submitted for p in STATE.match.players):
            asyncio.create_task(resolve_job(STATE.match))
        return


async def match_timeout_loop() -> None:
    while True:
        await asyncio.sleep(0.25)
        async with STATE.lock:
            match = STATE.match
            if match is None or match.phase != "betting":
                continue
            if time.time() >= match.deadline:
                asyncio.create_task(resolve_job(match))


async def client_loop(reader: asyncio.StreamReader, writer: asyncio.StreamWriter) -> None:
    player = Player(writer)
    try:
        while True:
            raw = await reader.readline()
            if not raw:
                break
            line = raw.decode("utf-8", errors="replace").strip()
            async with STATE.lock:
                await handle_line(player, line)
    except Exception:
        pass
    finally:
        async with STATE.lock:
            target = player.bound if player.bound is not None else player
            await drop_player(target)
        try:
            writer.close()
            await writer.wait_closed()
        except Exception:
            pass


async def main() -> None:
    asyncio.create_task(match_timeout_loop())
    server = await asyncio.start_server(client_loop, HOST, PORT)
    addrs = ", ".join(str(s.getsockname()) for s in server.sockets or [])
    print(f"MustDice server listening on {addrs}", flush=True)
    async with server:
        await server.serve_forever()


if __name__ == "__main__":
    asyncio.run(main())
