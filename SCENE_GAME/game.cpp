#include "game.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"
#include "run_session.h"
#include "bet_logic.h"
#include <cstdio>
#include <string>

using namespace DirectX;

enum BetKind {
	BET_KIND_NONE = 0,
	BET_KIND_PINPOINT,
	BET_KIND_ODD_EVEN,
};

static DrawFont* g_pPhaseText = nullptr;
static DrawFont* g_pStatusText = nullptr;
static DrawFont* g_pDetailText = nullptr;
static DrawFont* g_pHintText = nullptr;

static GamePhase g_phase = GAME_PHASE_ROUND_START;
static BetKind g_betKind = BET_KIND_NONE;
static int g_pickSum = BET_PINPOINT_CENTER;
static bool g_pickOdd = true;
static int g_die0 = 0;
static int g_die1 = 0;
static int g_rolledSum = 0;
static int g_lastScore = 0;
static char g_detailBuf[192] = {};

static const char* GamePhase_ToLabel(GamePhase phase)
{
	switch (phase)
	{
	case GAME_PHASE_ROUND_START: return "ROUND_START";
	case GAME_PHASE_BET_SELECT: return "BET_SELECT";
	case GAME_PHASE_PINPOINT_PICK: return "PINPOINT_PICK";
	case GAME_PHASE_ODD_EVEN_PICK: return "ODD_EVEN_PICK";
	case GAME_PHASE_ROLL: return "ROLL";
	case GAME_PHASE_RESOLVE: return "RESOLVE";
	case GAME_PHASE_GOAL_CHECK: return "GOAL_CHECK";
	default: return "UNKNOWN";
	}
}

static const char* GamePhase_ToHint(GamePhase phase)
{
	switch (phase)
	{
	case GAME_PHASE_ROUND_START:
		return "Enter,A: 賭け方選択へ";
	case GAME_PHASE_BET_SELECT:
		return "Enter,A: ピンポイント / BackSpace,B: 奇数偶数";
	case GAME_PHASE_PINPOINT_PICK:
		return "Left/Right: 予想変更 / Enter,A: 確定 / Esc,X: 戻る";
	case GAME_PHASE_ODD_EVEN_PICK:
		return "Enter,A: 奇数 / BackSpace,B: 偶数 / Esc,X: 戻る";
	case GAME_PHASE_ROLL:
		return "Enter,A: サイコロを振る / Esc,X: 戻る";
	case GAME_PHASE_RESOLVE:
		return "Enter,A: 次へ";
	case GAME_PHASE_GOAL_CHECK:
		return "Enter,A: 結果へ進む";
	default:
		return "Press Enter,A";
	}
}

static void Game_RefreshUi(void)
{
	if (!g_pPhaseText || !g_pStatusText || !g_pDetailText || !g_pHintText)
	{
		return;
	}

	char status[160] = {};
	std::snprintf(
		status,
		sizeof(status),
		"[Round%d]   score %d / %d  |  bet %d/%d  |  money %d",
		RunSession_GetRoundIndex(),
		RunSession_GetRoundScore(),
		RunSession_GetTargetScore(),
		RunSession_GetBetCount(),
		RUN_MAX_BETS_PER_ROUND,
		RunSession_GetMoney()
	);

	g_pPhaseText->SetText(std::string("GAME / ") + GamePhase_ToLabel(g_phase));
	g_pStatusText->SetText(status);
	g_pDetailText->SetText(g_detailBuf);
	g_pHintText->SetText(GamePhase_ToHint(g_phase));
}

static void Game_SetPhase(GamePhase phase)
{
	g_phase = phase;

	switch (phase)
	{
	case GAME_PHASE_ROUND_START:
		std::snprintf(
			g_detailBuf,
			sizeof(g_detailBuf),
			"目標スコア %d を目指せ（最大%d回賭け）",
			RunSession_GetTargetScore(),
			RUN_MAX_BETS_PER_ROUND
		);
		break;
	case GAME_PHASE_BET_SELECT:
		std::snprintf(g_detailBuf, sizeof(g_detailBuf), "賭け方を選ぶ");
		break;
	case GAME_PHASE_PINPOINT_PICK:
		std::snprintf(
			g_detailBuf,
			sizeof(g_detailBuf),
			"予想合計 = %d  (倍率 x%.1f)",
			g_pickSum,
			BetLogic_MultForSum(g_pickSum)
		);
		break;
	case GAME_PHASE_ODD_EVEN_PICK:
		std::snprintf(g_detailBuf, sizeof(g_detailBuf), "奇数か偶数かを選ぶ");
		break;
	case GAME_PHASE_ROLL:
		if (g_betKind == BET_KIND_PINPOINT)
		{
			std::snprintf(g_detailBuf, sizeof(g_detailBuf), "ピンポイント 予想合計 = %d で振る", g_pickSum);
		}
		else
		{
			std::snprintf(
				g_detailBuf,
				sizeof(g_detailBuf),
				"%s で振る",
				g_pickOdd ? "奇数" : "偶数"
			);
		}
		break;
	case GAME_PHASE_GOAL_CHECK:
		if (RunSession_IsTargetMet())
		{
			std::snprintf(
				g_detailBuf,
				sizeof(g_detailBuf),
				"クリア！ score %d >= %d",
				RunSession_GetRoundScore(),
				RunSession_GetTargetScore()
			);
		}
		else
		{
			std::snprintf(
				g_detailBuf,
				sizeof(g_detailBuf),
				"失敗… score %d < %d",
				RunSession_GetRoundScore(),
				RunSession_GetTargetScore()
			);
		}
		break;
	default:
		break;
	}

	Game_RefreshUi();
}

void Game_Initialize(void)
{
	// ラウンド開始は常に GAME 入場時に行う（Shop 側では進めない）
	RunSession_BeginRound();

	g_betKind = BET_KIND_NONE;
	g_pickSum = BET_PINPOINT_CENTER;
	g_pickOdd = true;
	g_die0 = 0;
	g_die1 = 0;
	g_rolledSum = 0;
	g_lastScore = 0;
	g_detailBuf[0] = '\0';

	g_pPhaseText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, 70.0f },
		20.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"GAME"
	);

	g_pStatusText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 3.0f },
		40.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		""
	);

	g_pDetailText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f},
	30.0f,
		0.0f,
		{ 1.0f, 1.0f, 0.85f, 1.0f },
		""
	);

	g_pHintText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 110.0f },
		30.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		""
	);

	Game_SetPhase(GAME_PHASE_ROUND_START);
}

void Game_Update(void)
{
	const bool decide = Input_IsActionTrigger(INPUT_ACTION_DECIDE);
	const bool cancel = Input_IsActionTrigger(INPUT_ACTION_CANCEL);
	const bool back = Input_IsActionTrigger(INPUT_ACTION_BACK);
	const bool left = Input_IsActionTrigger(INPUT_ACTION_MENU_LEFT);
	const bool right = Input_IsActionTrigger(INPUT_ACTION_MENU_RIGHT);

	switch (g_phase)
	{
	case GAME_PHASE_ROUND_START:
		if (decide)
		{
			Game_SetPhase(GAME_PHASE_BET_SELECT);
		}
		break;

	case GAME_PHASE_BET_SELECT:
		if (decide)
		{
			g_betKind = BET_KIND_PINPOINT;
			g_pickSum = BET_PINPOINT_CENTER;
			Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
		}
		else if (cancel)
		{
			g_betKind = BET_KIND_ODD_EVEN;
			Game_SetPhase(GAME_PHASE_ODD_EVEN_PICK);
		}
		break;

	case GAME_PHASE_PINPOINT_PICK:
		if (back)
		{
			g_betKind = BET_KIND_NONE;
			Game_SetPhase(GAME_PHASE_BET_SELECT);
		}
		else if (left && g_pickSum > BET_SUM_MIN)
		{
			g_pickSum -= 1;
			Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
		}
		else if (right && g_pickSum < BET_SUM_MAX)
		{
			g_pickSum += 1;
			Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
		}
		else if (decide)
		{
			Game_SetPhase(GAME_PHASE_ROLL);
		}
		break;

	case GAME_PHASE_ODD_EVEN_PICK:
		if (back)
		{
			g_betKind = BET_KIND_NONE;
			Game_SetPhase(GAME_PHASE_BET_SELECT);
		}
		else if (decide)
		{
			g_pickOdd = true;
			Game_SetPhase(GAME_PHASE_ROLL);
		}
		else if (cancel)
		{
			g_pickOdd = false;
			Game_SetPhase(GAME_PHASE_ROLL);
		}
		break;

	case GAME_PHASE_ROLL:
		if (back)
		{
			if (g_betKind == BET_KIND_PINPOINT)
			{
				Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
			}
			else if (g_betKind == BET_KIND_ODD_EVEN)
			{
				Game_SetPhase(GAME_PHASE_ODD_EVEN_PICK);
			}
			else
			{
				Game_SetPhase(GAME_PHASE_BET_SELECT);
			}
		}
		else if (decide)
		{
			BetLogic_Roll2d6(&g_die0, &g_die1);
			g_rolledSum = g_die0 + g_die1;

			if (g_betKind == BET_KIND_PINPOINT)
			{
				g_lastScore = BetLogic_ScorePinpoint(g_pickSum, g_rolledSum);
				const int diff = (g_pickSum > g_rolledSum) ? (g_pickSum - g_rolledSum) : (g_rolledSum - g_pickSum);
				std::snprintf(
					g_detailBuf,
					sizeof(g_detailBuf),
					"予想合計 = %d / 出目「%d+%d=%d」/ 基礎倍率:x%.1f - %.1f / %+d",
					g_pickSum,
					g_die0,
					g_die1,
					g_rolledSum,
					BetLogic_MultForSum(g_pickSum),
					BET_PINPOINT_STEP * static_cast<float>(diff),
					g_lastScore
				);
			}
			else
			{
				g_lastScore = BetLogic_ScoreOddEven(g_pickOdd, g_rolledSum);
				const bool hit = (BetLogic_IsOdd(g_rolledSum) == g_pickOdd);
				std::snprintf(
					g_detailBuf,
					sizeof(g_detailBuf),
					"%s / 出目「%d+%d=%d」 / 【%s】x%.1f / %+d",
					g_pickOdd ? "奇数を選択" : "偶数を選択",
					g_die0,
					g_die1,
					g_rolledSum,
					hit ? "HIT!!!" : "MISS……",
					BetLogic_OddEvenMult(hit),
					g_lastScore
				);
			}

			g_phase = GAME_PHASE_RESOLVE;
			RunSession_ApplyBetScore(g_lastScore);
			Game_RefreshUi();
		}
		break;

	case GAME_PHASE_RESOLVE:
		if (decide)
		{
			if (RunSession_IsBetLimitReached())
			{
				Game_SetPhase(GAME_PHASE_GOAL_CHECK);
			}
			else
			{
				g_betKind = BET_KIND_NONE;
				Game_SetPhase(GAME_PHASE_BET_SELECT);
			}
		}
		break;

	case GAME_PHASE_GOAL_CHECK:
		if (decide && GetFadeState() == FADE_NONE)
		{
			if (RunSession_IsTargetMet())
			{
				RunSession_GrantClearReward();
				SetSceneFade(SCENE_SHOP);
			}
			else
			{
				SetSceneFade(SCENE_RESULT);
			}
		}
		break;

	default:
		break;
	}
}

void Game_Draw(void)
{
	if (g_pPhaseText) g_pPhaseText->Draw();
	if (g_pStatusText) g_pStatusText->Draw();
	if (g_pDetailText) g_pDetailText->Draw();
	if (g_pHintText) g_pHintText->Draw();
}

void Game_Finalize(void)
{
	SAFE_DELETE(g_pPhaseText);
	SAFE_DELETE(g_pStatusText);
	SAFE_DELETE(g_pDetailText);
	SAFE_DELETE(g_pHintText);
}
