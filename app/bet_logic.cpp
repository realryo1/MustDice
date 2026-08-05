#include "bet_logic.h"
#include <cmath>
#include <random>

static std::mt19937 g_rng;
static bool g_rngReady = false;

void BetLogic_SeedFromDevice(void)
{
	std::random_device rd;
	g_rng.seed(rd());
	g_rngReady = true;
}

void BetLogic_SetSeed(unsigned int seed)
{
	g_rng.seed(seed);
	g_rngReady = true;
}

static void BetLogic_EnsureRng(void)
{
	if (!g_rngReady)
	{
		BetLogic_SeedFromDevice();
	}
}

float BetLogic_MultForSum(int sum)
{
	if (sum < BET_SUM_MIN) sum = BET_SUM_MIN;
	if (sum > BET_SUM_MAX) sum = BET_SUM_MAX;

	const int distFromCenter = (sum > 7) ? (sum - 7) : (7 - sum);
	return 1.0f + 0.1f * static_cast<float>(distFromCenter);
}

void BetLogic_Roll2d6(int* outDie0, int* outDie1)
{
	BetLogic_EnsureRng();
	std::uniform_int_distribution<int> dist(1, 6);
	if (outDie0) *outDie0 = dist(g_rng);
	if (outDie1) *outDie1 = dist(g_rng);
}

static int BetLogic_RoundToInt(float value)
{
	return static_cast<int>(std::lround(value));
}

int BetLogic_ScorePinpoint(int pickSum, int rolledSum)
{
	const float mult = BetLogic_MultForSum(pickSum);
	const int diff = (pickSum > rolledSum) ? (pickSum - rolledSum) : (rolledSum - pickSum);
	const float factor = mult - 0.1f * static_cast<float>(diff);
	return BetLogic_RoundToInt(static_cast<float>(BET_BASE_SCORE) * factor);
}

int BetLogic_ScoreOddEven(bool pickOdd, int rolledSum)
{
	const bool hit = (BetLogic_IsOdd(rolledSum) == pickOdd);
	const float factor = hit ? 1.2f : 0.6f;
	return BetLogic_RoundToInt(static_cast<float>(BET_BASE_SCORE) * factor);
}

bool BetLogic_IsOdd(int sum)
{
	return (sum % 2) != 0;
}
