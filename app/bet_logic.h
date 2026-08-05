#pragma once

// 賭けの純計算・2d6（ルール正本: document_ingame/about_game.md）

static const int BET_BASE_SCORE = 100;
static const int BET_SUM_MIN = 2;
static const int BET_SUM_MAX = 12;

void BetLogic_SeedFromDevice(void);
void BetLogic_SetSeed(unsigned int seed);

float BetLogic_MultForSum(int sum);
void BetLogic_Roll2d6(int* outDie0, int* outDie1);

// 整数スコア（四捨五入）
int BetLogic_ScorePinpoint(int pickSum, int rolledSum);
int BetLogic_ScoreOddEven(bool pickOdd, int rolledSum);

bool BetLogic_IsOdd(int sum);
