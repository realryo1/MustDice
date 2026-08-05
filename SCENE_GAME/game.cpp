#include "game.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"

using namespace DirectX;

static DrawFont* g_pGameText = nullptr;
static DrawFont* g_pHintText = nullptr;

void Game_Initialize(void)
{
	g_pGameText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 40.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"GAME"
	);

	g_pHintText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 40.0f },
		28.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"Press Decide"
	);
}

void Game_Update(void)
{
	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE))
	{
		SetSceneFade(SCENE_SHOP);
	}
}

void Game_Draw(void)
{
	if (g_pGameText) g_pGameText->Draw();
	if (g_pHintText) g_pHintText->Draw();
}

void Game_Finalize(void)
{
	delete g_pGameText; g_pGameText = nullptr;
	delete g_pHintText; g_pHintText = nullptr;
}
