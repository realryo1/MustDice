#include "run_session.h"

// α用プレースホルダ定数（本スコア計算は未実装）
static const int kInitialTargetScore = 200;
static const int kTargetScoreStep = 50;
static const int kMaxBetsPerRound = 3;
static const int kStubBetScore = 100;

static int g_roundIndex = 0;
static int g_targetScore = kInitialTargetScore;
static int g_roundScore = 0;
static int g_betCount = 0;
static int g_money = 0;

void RunSession_Reset(void)
{
	g_roundIndex = 0;
	g_targetScore = kInitialTargetScore;
	g_roundScore = 0;
	g_betCount = 0;
	g_money = 0;
}

void RunSession_BeginRound(void)
{
	g_roundIndex += 1;
	g_roundScore = 0;
	g_betCount = 0;

	if (g_roundIndex <= 1)
	{
		g_targetScore = kInitialTargetScore;
	}
	else
	{
		g_targetScore += kTargetScoreStep;
	}
}

void RunSession_AddBetStub(void)
{
	// 賭けロジック未実装。進行確認用に固定点を加算するだけ
	g_roundScore += kStubBetScore;
	g_betCount += 1;
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
