#include "shop.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"
#include "run_session.h"
#include <cstdio>

using namespace DirectX;

static DrawFont* g_pShopText = nullptr;
static DrawFont* g_pStatusText = nullptr;
static DrawFont* g_pHintText = nullptr;

void Shop_Initialize(void)
{
	// α: 購入・強化は未実装。クリア報酬確認のみ

	char status[160] = {};
	std::snprintf(
		status,
		sizeof(status),
		"R%d clear  +%d money  total %d  next target %d",
		RunSession_GetRoundIndex(),
		RunSession_GetLastClearReward(),
		RunSession_GetMoney(),
		RunSession_GetTargetScore() + RUN_TARGET_SCORE_STEP
	);

	g_pShopText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 60.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"SHOP"
	);

	g_pStatusText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		22.0f,
		0.0f,
		{ 0.9f, 0.9f, 0.9f, 1.0f },
		status
	);

	g_pHintText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 50.0f },
		22.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"Decide: 次ラウンドへ（購入はα未実装）"
	);
}

void Shop_Update(void)
{
	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE))
	{
		RunSession_BeginRound();
		SetSceneFade(SCENE_GAME);
	}
}

void Shop_Draw(void)
{
	if (g_pShopText) g_pShopText->Draw();
	if (g_pStatusText) g_pStatusText->Draw();
	if (g_pHintText) g_pHintText->Draw();
}

void Shop_Finalize(void)
{
	SAFE_DELETE(g_pShopText);
	SAFE_DELETE(g_pStatusText);
	SAFE_DELETE(g_pHintText);
}
