#pragma once

enum GamePhase {
	GAME_PHASE_ROUND_START = 0,
	GAME_PHASE_BET_SELECT,
	GAME_PHASE_PINPOINT_PICK,
	GAME_PHASE_ODD_EVEN_PICK,
	GAME_PHASE_ROLL,
	GAME_PHASE_RESOLVE_STUB,
	GAME_PHASE_GOAL_CHECK_STUB,
};

void Game_Initialize(void);
void Game_Finalize(void);
void Game_Update(void);
void Game_Draw(void);
