#include "result.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"
#include "run_session.h"
#include <cstdio>
#include <string>

using namespace DirectX;

static DrawFont* g_pResultText = nullptr;
static DrawFont* g_pStatusText = nullptr;
static DrawFont* g_pHintText = nullptr;

void Result_Initialize(void)
{
	char status[128] = {};
	std::snprintf(
		status,
		sizeof(status),
		"R%d  score %d / %d",
		RunSession_GetRoundIndex(),
		RunSession_GetRoundScore(),
		RunSession_GetTargetScore()
	);

	g_pResultText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 60.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"GAME OVER"
	);

	g_pStatusText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f },
		24.0f,
		0.0f,
		{ 0.9f, 0.9f, 0.9f, 1.0f },
		status
	);

	g_pHintText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 50.0f },
		22.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"Decide: タイトルへ"
	);
}

void Result_Update(void)
{
	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE))
	{
		RunSession_Reset();
		SetSceneFade(SCENE_TITLE);
	}
}

void Result_Draw(void)
{
	if (g_pResultText) g_pResultText->Draw();
	if (g_pStatusText) g_pStatusText->Draw();
	if (g_pHintText) g_pHintText->Draw();
}

void Result_Finalize(void)
{
	SAFE_DELETE(g_pResultText);
	SAFE_DELETE(g_pStatusText);
	SAFE_DELETE(g_pHintText);
}
