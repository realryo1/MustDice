#include "game.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"
#include "run_session.h"
#include <cstdio>
#include <string>

using namespace DirectX;

static DrawFont* g_pPhaseText = nullptr;
static DrawFont* g_pStatusText = nullptr;
static DrawFont* g_pHintText = nullptr;
static GamePhase g_phase = GAME_PHASE_ROUND_START;

static const char* GamePhase_ToLabel(GamePhase phase)
{
	switch (phase)
	{
	case GAME_PHASE_ROUND_START: return "ROUND_START";
	case GAME_PHASE_BET_SELECT: return "BET_SELECT";
	case GAME_PHASE_PINPOINT_PICK: return "PINPOINT_PICK";
	case GAME_PHASE_ODD_EVEN_PICK: return "ODD_EVEN_PICK";
	case GAME_PHASE_ROLL: return "ROLL";
	case GAME_PHASE_RESOLVE_STUB: return "RESOLVE_STUB";
	case GAME_PHASE_GOAL_CHECK_STUB: return "GOAL_CHECK_STUB";
	default: return "UNKNOWN";
	}
}

static const char* GamePhase_ToHint(GamePhase phase)
{
	switch (phase)
	{
	case GAME_PHASE_ROUND_START:
		return "Decide: 賭け方選択へ";
	case GAME_PHASE_BET_SELECT:
		return "Decide: ピンポイント / Cancel: 奇数偶数";
	case GAME_PHASE_PINPOINT_PICK:
	case GAME_PHASE_ODD_EVEN_PICK:
		return "Decide: サイコロを振る（stub）";
	case GAME_PHASE_ROLL:
		return "Decide: 結果へ（stub）";
	case GAME_PHASE_RESOLVE_STUB:
		return "Decide: 次へ";
	case GAME_PHASE_GOAL_CHECK_STUB:
		return "Decide: ショップ / Cancel: ゲームオーバー";
	default:
		return "Press Decide";
	}
}

static void Game_RefreshUi(void)
{
	if (!g_pPhaseText || !g_pStatusText || !g_pHintText)
	{
		return;
	}

	char status[160] = {};
	std::snprintf(
		status,
		sizeof(status),
		"R%d  score %d / %d  bet %d/3  money %d",
		RunSession_GetRoundIndex(),
		RunSession_GetRoundScore(),
		RunSession_GetTargetScore(),
		RunSession_GetBetCount(),
		RunSession_GetMoney()
	);

	g_pPhaseText->SetText(std::string("GAME / ") + GamePhase_ToLabel(g_phase));
	g_pStatusText->SetText(status);
	g_pHintText->SetText(GamePhase_ToHint(g_phase));
}

static void Game_SetPhase(GamePhase phase)
{
	g_phase = phase;
	Game_RefreshUi();
}

void Game_Initialize(void)
{
	// Title から入場時は roundIndex==0。Shop 経由の再入場は BeginRound 済み想定
	if (RunSession_GetRoundIndex() <= 0)
	{
		RunSession_BeginRound();
	}

	g_phase = GAME_PHASE_ROUND_START;

	g_pPhaseText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 60.0f },
		40.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"GAME"
	);

	g_pStatusText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		24.0f,
		0.0f,
		{ 0.9f, 0.9f, 0.9f, 1.0f },
		""
	);

	g_pHintText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 50.0f },
		22.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		""
	);

	Game_RefreshUi();
}

void Game_Update(void)
{
	const bool decide = Input_IsActionTrigger(INPUT_ACTION_DECIDE);
	const bool cancel = Input_IsActionTrigger(INPUT_ACTION_CANCEL);

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
			Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
		}
		else if (cancel)
		{
			Game_SetPhase(GAME_PHASE_ODD_EVEN_PICK);
		}
		break;

	case GAME_PHASE_PINPOINT_PICK:
	case GAME_PHASE_ODD_EVEN_PICK:
		if (decide)
		{
			Game_SetPhase(GAME_PHASE_ROLL);
		}
		break;

	case GAME_PHASE_ROLL:
		if (decide)
		{
			Game_SetPhase(GAME_PHASE_RESOLVE_STUB);
		}
		break;

	case GAME_PHASE_RESOLVE_STUB:
		if (decide)
		{
			RunSession_AddBetStub();
			if (RunSession_IsBetLimitReached())
			{
				Game_SetPhase(GAME_PHASE_GOAL_CHECK_STUB);
			}
			else
			{
				Game_SetPhase(GAME_PHASE_BET_SELECT);
			}
		}
		break;

	case GAME_PHASE_GOAL_CHECK_STUB:
		if (decide)
		{
			// クリア stub → ショップ（お金加算などは未実装）
			SetSceneFade(SCENE_SHOP);
		}
		else if (cancel)
		{
			SetSceneFade(SCENE_RESULT);
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
	if (g_pHintText) g_pHintText->Draw();
}

void Game_Finalize(void)
{
	SAFE_DELETE(g_pPhaseText);
	SAFE_DELETE(g_pStatusText);
	SAFE_DELETE(g_pHintText);
}
