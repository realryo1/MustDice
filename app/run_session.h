#pragma once

// シーンをまたぐラン進行データ
// 詳細: document_ingame/run_session.md

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
