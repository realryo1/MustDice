#include "match_session.h"
#include "net_client.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

static MatchSession g_match = {};

void MatchSession_Reset(void)
{
	std::memset(&g_match, 0, sizeof(g_match));
	g_match.myId = -1;
}

MatchSession* MatchSession_Get(void)
{
	return &g_match;
}

static std::vector<std::string> Split(const char* line)
{
	std::vector<std::string> out;
	std::istringstream ss(line ? line : "");
	std::string tok;
	while (ss >> tok)
	{
		out.push_back(tok);
	}
	return out;
}

static int ToInt(const std::string& s)
{
	return std::atoi(s.c_str());
}

void MatchSession_ApplyLine(const char* line)
{
	const std::vector<std::string> t = Split(line);
	if (t.empty())
	{
		return;
	}
	const std::string& cmd = t[0];
	if (cmd == "WELCOME" && t.size() >= 3)
	{
		g_match.myId = ToInt(t[1]);
		g_match.queue = ToInt(t[2]);
		g_match.lastErr = 0;
	}
	else if (cmd == "LOBBY" && t.size() >= 4)
	{
		g_match.playerCount = ToInt(t[1]);
		g_match.readyMask = ToInt(t[2]);
		g_match.waitingBehind = ToInt(t[3]);
		g_match.inMatch = 0;
		for (int i = 0; i < MATCH_MAX_PLAYERS; ++i)
		{
			g_match.players[i].name[0] = '\0';
		}
		for (int i = 0; i < g_match.playerCount && (4 + i) < static_cast<int>(t.size()); ++i)
		{
			std::snprintf(g_match.players[i].name, sizeof(g_match.players[i].name), "%s", t[4 + i].c_str());
			g_match.players[i].playerId = i;
		}
	}
	else if (cmd == "MATCH_START" && t.size() >= 2)
	{
		g_match.inMatch = 1;
		g_match.matchEnded = 0;
		g_match.playerCount = ToInt(t[1]);
		g_match.currentRound = 1;
		g_match.currentBet = 1;
	}
	else if (cmd == "BET_OPEN" && t.size() >= 4)
	{
		g_match.currentRound = ToInt(t[1]);
		g_match.currentBet = ToInt(t[2]);
		g_match.remainSec = ToInt(t[3]);
		g_match.submittedMask = 0;
		g_match.inMatch = 1;
		g_match.roundBoard = 0;
		for (int i = 0; i < MATCH_MAX_PLAYERS; ++i)
		{
			g_match.players[i].lastRank = 0;
		}
	}
	else if (cmd == "BET_WAIT" && t.size() >= 2)
	{
		g_match.submittedMask = ToInt(t[1]);
	}
	else if (cmd == "RESOLVE" && t.size() >= 2)
	{
		const int n = ToInt(t[1]);
		int cursor = 2;
		for (int i = 0; i < n && cursor + 7 < static_cast<int>(t.size()); ++i)
		{
			const int id = ToInt(t[cursor]);
			if (id >= 0 && id < MATCH_MAX_PLAYERS)
			{
				MatchPlayerView& p = g_match.players[id];
				p.playerId = id;
				p.kind = (t[cursor + 1] == "P") ? 1 : 2;
				p.value = ToInt(t[cursor + 2]);
				p.die0 = ToInt(t[cursor + 3]);
				p.die1 = ToInt(t[cursor + 4]);
				p.lastScore = ToInt(t[cursor + 5]);
				p.roundScore = ToInt(t[cursor + 6]);
				p.autoSelect = ToInt(t[cursor + 7]);
			}
			cursor += 8;
		}
		g_match.playerCount = n;
	}
	else if (cmd == "ROUND_END" && t.size() >= 3)
	{
		const int n = ToInt(t[1]);
		int cursor = 3;
		for (int i = 0; i < n && cursor + 5 < static_cast<int>(t.size()); ++i)
		{
			const int id = ToInt(t[cursor]);
			if (id >= 0 && id < MATCH_MAX_PLAYERS)
			{
				MatchPlayerView& p = g_match.players[id];
				p.lastRank = ToInt(t[cursor + 1]);
				p.matchPointX100 = ToInt(t[cursor + 2]);
				p.firstPlaceCount = ToInt(t[cursor + 3]);
				p.totalRoundScore = ToInt(t[cursor + 4]);
				p.roundScore = ToInt(t[cursor + 5]);
			}
			cursor += 6;
		}
		g_match.roundBoard = 1;
	}
	else if (cmd == "MATCH_END" && t.size() >= 2)
	{
		const int n = ToInt(t[1]);
		int cursor = 2;
		for (int i = 0; i < n && cursor + 4 < static_cast<int>(t.size()); ++i)
		{
			const int id = ToInt(t[cursor]);
			if (id >= 0 && id < MATCH_MAX_PLAYERS)
			{
				MatchPlayerView& p = g_match.players[id];
				p.lastRank = ToInt(t[cursor + 1]);
				p.matchPointX100 = ToInt(t[cursor + 2]);
				p.firstPlaceCount = ToInt(t[cursor + 3]);
				p.totalRoundScore = ToInt(t[cursor + 4]);
			}
			cursor += 5;
		}
		g_match.matchEnded = 1;
		g_match.inMatch = 0;
	}
	else if (cmd == "SNAP" && t.size() >= 9)
	{
		g_match.myId = ToInt(t[1]);
		g_match.currentRound = ToInt(t[2]);
		g_match.currentBet = ToInt(t[3]);
		g_match.remainSec = ToInt(t[4]);
		g_match.submittedMask = ToInt(t[5]);
		g_match.playerCount = ToInt(t[6]);
		if (g_match.myId >= 0 && g_match.myId < MATCH_MAX_PLAYERS)
		{
			g_match.players[g_match.myId].roundScore = ToInt(t[7]);
			g_match.players[g_match.myId].matchPointX100 = ToInt(t[8]);
		}
		g_match.inMatch = 1;
		for (int i = 0; i < g_match.playerCount && (9 + i) < static_cast<int>(t.size()); ++i)
		{
			std::snprintf(g_match.players[i].name, sizeof(g_match.players[i].name), "%s", t[9 + i].c_str());
		}
	}
	else if (cmd == "ERR")
	{
		g_match.lastErr = (t.size() >= 2 && t[1] == "NAME") ? 1 : 2;
	}
}

void MatchSession_PollNet(void)
{
	NetClient_Poll();
	std::string line;
	while (NetClient_PopLine(line))
	{
		MatchSession_ApplyLine(line.c_str());
	}
}
