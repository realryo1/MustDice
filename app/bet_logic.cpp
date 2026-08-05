#include "bet_logic.h"
#include <cmath>
#include <random>

static std::mt19937 g_rng;
static bool g_rngReady = false;

// ハードウェア乱数でサイコロ用RNGを初期化する
void BetLogic_SeedFromDevice(void)
{
	std::random_device rd;
	g_rng.seed(rd());
	g_rngReady = true;
}

// 指定シードでサイコロ用RNGを初期化する
void BetLogic_SetSeed(unsigned int seed)
{
	g_rng.seed(seed);
	g_rngReady = true;
}

// RNG未初期化ならデバイスシードで用意する
static void BetLogic_EnsureRng(void)
{
	if (!g_rngReady)
	{
		BetLogic_SeedFromDevice();
	}
}

// 合計出目に対応するピンポイント倍率を返す
float BetLogic_MultForSum(int sum)
{
	if (sum < BET_SUM_MIN) sum = BET_SUM_MIN;
	if (sum > BET_SUM_MAX) sum = BET_SUM_MAX;

	const int distFromCenter = (sum > BET_PINPOINT_CENTER)
		? (sum - BET_PINPOINT_CENTER)
		: (BET_PINPOINT_CENTER - sum);
	return BET_PINPOINT_BASE_MULT + BET_PINPOINT_STEP * static_cast<float>(distFromCenter);
}

// 奇数/偶数賭けの的中・外れ倍率を返す
float BetLogic_OddEvenMult(bool hit)
{
	return hit ? BET_ODD_EVEN_HIT_MULT : BET_ODD_EVEN_MISS_MULT;
}

// 6面サイコロを2個振り、各出目を書き出す
void BetLogic_Roll2d6(int* outDie0, int* outDie1)
{
	BetLogic_EnsureRng();
	std::uniform_int_distribution<int> dist(BET_DIE_FACE_MIN, BET_DIE_FACE_MAX);
	if (outDie0) *outDie0 = dist(g_rng);
	if (outDie1) *outDie1 = dist(g_rng);
}

// 浮動小数スコアを四捨五入して整数にする
static int BetLogic_RoundToInt(float value)
{
	return static_cast<int>(std::lround(value));
}

// ピンポイント賭けの得点を計算する
int BetLogic_ScorePinpoint(int pickSum, int rolledSum)
{
	const float mult = BetLogic_MultForSum(pickSum);
	const int diff = (pickSum > rolledSum) ? (pickSum - rolledSum) : (rolledSum - pickSum);
	const float factor = mult - BET_PINPOINT_STEP * static_cast<float>(diff);
	return BetLogic_RoundToInt(static_cast<float>(BET_BASE_SCORE) * factor);
}

// 奇数/偶数賭けの得点を計算する
int BetLogic_ScoreOddEven(bool pickOdd, int rolledSum)
{
	const bool hit = (BetLogic_IsOdd(rolledSum) == pickOdd);
	const float factor = BetLogic_OddEvenMult(hit);
	return BetLogic_RoundToInt(static_cast<float>(BET_BASE_SCORE) * factor);
}

// 合計が奇数かどうかを返す
bool BetLogic_IsOdd(int sum)
{
	return (sum % 2) != 0;
}
