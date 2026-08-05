#include "run_session.h"
#include "bet_logic.h"

static int g_roundIndex = 0;
static int g_targetScore = RUN_INITIAL_TARGET_SCORE;
static int g_roundScore = 0;
static int g_betCount = 0;
static int g_money = 0;
static int g_lastClearReward = 0;

// ラン全体を初期化し、乱数も再シードする
void RunSession_Reset(void)
{
	g_roundIndex = 0;
	g_targetScore = RUN_INITIAL_TARGET_SCORE;
	g_roundScore = 0;
	g_betCount = 0;
	g_money = 0;
	g_lastClearReward = 0;
	BetLogic_SeedFromDevice();
}

// 次ラウンドを開始し、目標スコアを更新する
void RunSession_BeginRound(void)
{
	g_roundIndex += 1;
	g_roundScore = 0;
	g_betCount = 0;
	g_lastClearReward = 0;

	if (g_roundIndex <= 1)
	{
		g_targetScore = RUN_INITIAL_TARGET_SCORE;
	}
	else
	{
		g_targetScore += RUN_TARGET_SCORE_STEP;
	}
}

// 1回の賭けスコアを合計に加算し、賭け回数を進める
void RunSession_ApplyBetScore(int score)
{
	g_roundScore += score;
	g_betCount += 1;
}

// クリア報酬を計算して所持金に加算する
void RunSession_GrantClearReward(void)
{
	g_lastClearReward = g_roundScore / RUN_CLEAR_REWARD_DIVISOR;
	g_money += g_lastClearReward;
}

// 現在のラウンド番号を返す
int RunSession_GetRoundIndex(void)
{
	return g_roundIndex;
}

// 現在ラウンドの目標スコアを返す
int RunSession_GetTargetScore(void)
{
	return g_targetScore;
}

// 現在ラウンドの合計スコアを返す
int RunSession_GetRoundScore(void)
{
	return g_roundScore;
}

// 現在ラウンドの賭け回数を返す
int RunSession_GetBetCount(void)
{
	return g_betCount;
}

// 所持金を返す
int RunSession_GetMoney(void)
{
	return g_money;
}

// 直前のクリア報酬額を返す
int RunSession_GetLastClearReward(void)
{
	return g_lastClearReward;
}

// 目標スコアを直接設定する
void RunSession_SetTargetScore(int value)
{
	g_targetScore = value;
}

// ラウンド合計スコアを直接設定する
void RunSession_SetRoundScore(int value)
{
	g_roundScore = value;
}

// 賭け回数を直接設定する
void RunSession_SetBetCount(int value)
{
	g_betCount = value;
}

// 所持金を直接設定する
void RunSession_SetMoney(int value)
{
	g_money = value;
}

// 今ラウンドの賭け上限に達したか判定する
bool RunSession_IsBetLimitReached(void)
{
	return g_betCount >= RUN_MAX_BETS_PER_ROUND;
}

// ラウンド合計が目標スコア以上か判定する
bool RunSession_IsTargetMet(void)
{
	return g_roundScore >= g_targetScore;
}
