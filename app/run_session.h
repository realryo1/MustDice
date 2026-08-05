#pragma once

// シーンをまたぐラン進行データ
// 詳細: document_ingame/run_session.md

static const int RUN_INITIAL_TARGET_SCORE = 200; // 1ラウンド目の目標スコア
static const int RUN_TARGET_SCORE_STEP = 50;     // ラウンド進行ごとの目標増加量
static const int RUN_MAX_BETS_PER_ROUND = 3;     // 1ラウンドあたりの最大賭け回数
static const int RUN_CLEAR_REWARD_DIVISOR = 10;  // クリア報酬 = roundScore / この値

void RunSession_Reset(void);
void RunSession_BeginRound(void);
void RunSession_ApplyBetScore(int score);
void RunSession_GrantClearReward(void);

int RunSession_GetRoundIndex(void);
int RunSession_GetTargetScore(void);
int RunSession_GetRoundScore(void);
int RunSession_GetBetCount(void);
int RunSession_GetMoney(void);
int RunSession_GetLastClearReward(void);

void RunSession_SetTargetScore(int value);
void RunSession_SetRoundScore(int value);
void RunSession_SetBetCount(int value);
void RunSession_SetMoney(int value);

bool RunSession_IsBetLimitReached(void);
bool RunSession_IsTargetMet(void);
