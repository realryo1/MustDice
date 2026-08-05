#pragma once

// 賭けの純計算・2d6（ルール正本: document_ingame/about_game.md）

static const int BET_BASE_SCORE = 100;           // 素点
static const int BET_SUM_MIN = 2;                // 2d6 合計の下限
static const int BET_SUM_MAX = 12;               // 2d6 合計の上限
static const int BET_PINPOINT_CENTER = 7;        // ピンポイント倍率の中央値（最頻出）
static const float BET_PINPOINT_BASE_MULT = 1.0f; // 中央値のときの倍率
static const float BET_PINPOINT_STEP = 0.1f;     // 中央から1離れる／外れ1ごとの倍率差分
static const float BET_ODD_EVEN_HIT_MULT = 1.2f; // 奇数偶数の的中倍率
static const float BET_ODD_EVEN_MISS_MULT = 0.6f; // 奇数偶数の外れ倍率
static const int BET_DIE_FACE_MIN = 1;           // サイコロ1個の出目下限
static const int BET_DIE_FACE_MAX = 6;           // サイコロ1個の出目上限

void BetLogic_SeedFromDevice(void);
void BetLogic_SetSeed(unsigned int seed);

float BetLogic_MultForSum(int sum);
float BetLogic_OddEvenMult(bool hit);
void BetLogic_Roll2d6(int* outDie0, int* outDie1);

// 整数スコア（四捨五入）
int BetLogic_ScorePinpoint(int pickSum, int rolledSum);
int BetLogic_ScoreOddEven(bool pickOdd, int rolledSum);

bool BetLogic_IsOdd(int sum);
