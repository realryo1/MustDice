#pragma once

static const int MATCH_MAX_PLAYERS = 4;

struct MatchPlayerView
{
	char name[24];
	int playerId;
	int roundScore;
	int totalRoundScore;
	int matchPointX100;
	int firstPlaceCount;
	int lastRank;
	int lastScore;
	int die0;
	int die1;
	int kind;   // 0 none, 1 P, 2 O
	int value;
	int autoSelect;
	int submitted;
	int connected;
};

struct MatchSession
{
	int myId;
	int queue;
	int playerCount;
	int readyMask;
	int waitingBehind;
	int submittedMask;
	int currentRound;
	int currentBet;
	int remainSec;
	int inMatch;
	int matchEnded;
	int roundBoard;
	int lastErr; // 1 NAME, 2 other
	int die0;
	int die1;
	MatchPlayerView players[MATCH_MAX_PLAYERS];
};

void MatchSession_Reset(void);
MatchSession* MatchSession_Get(void);
void MatchSession_ApplyLine(const char* line);
void MatchSession_PollNet(void);
