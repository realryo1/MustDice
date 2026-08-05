#include "run_session.h"
#include "bet_logic.h"

static const int kInitialTargetScore = 200;
static const int kTargetScoreStep = 50;
static const int kMaxBetsPerRound = 3;

static int g_roundIndex = 0;
static int g_targetScore = kInitialTargetScore;
static int g_roundScore = 0;
static int g_betCount = 0;
static int g_money = 0;
static int g_lastClearReward = 0;

void RunSession_Reset(void)
{
	g_roundIndex = 0;
	g_targetScore = kInitialTargetScore;
	g_roundScore = 0;
	g_betCount = 0;
	g_money = 0;
	g_lastClearReward = 0;
	BetLogic_SeedFromDevice();
}

void RunSession_BeginRound(void)
{
	g_roundIndex += 1;
	g_roundScore = 0;
	g_betCount = 0;
	g_lastClearReward = 0;

	if (g_roundIndex <= 1)
	{
		g_targetScore = kInitialTargetScore;
	}
	else
	{
		g_targetScore += kTargetScoreStep;
	}
}

void RunSession_ApplyBetScore(int score)
{
	g_roundScore += score;
	g_betCount += 1;
}

void RunSession_GrantClearReward(void)
{
	g_lastClearReward = g_roundScore / 10;
	g_money += g_lastClearReward;
}

int RunSession_GetRoundIndex(void)
{
	return g_roundIndex;
}

int RunSession_GetTargetScore(void)
{
	return g_targetScore;
}

int RunSession_GetRoundScore(void)
{
	return g_roundScore;
}

int RunSession_GetBetCount(void)
{
	return g_betCount;
}

int RunSession_GetMoney(void)
{
	return g_money;
}

int RunSession_GetLastClearReward(void)
{
	return g_lastClearReward;
}

void RunSession_SetTargetScore(int value)
{
	g_targetScore = value;
}

void RunSession_SetRoundScore(int value)
{
	g_roundScore = value;
}

void RunSession_SetBetCount(int value)
{
	g_betCount = value;
}

void RunSession_SetMoney(int value)
{
	g_money = value;
}

bool RunSession_IsBetLimitReached(void)
{
	return g_betCount >= kMaxBetsPerRound;
}

bool RunSession_IsTargetMet(void)
{
	return g_roundScore >= g_targetScore;
}
