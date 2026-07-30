#include "title.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"

using namespace DirectX;

static Sprite2D* g_pNaiyo = nullptr;
static FontRenderer* g_pTitleText = nullptr;
static FontRenderer* g_pHintText = nullptr;

void Title_Initialize(void)
{
	g_pTitleText = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 40.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"TITLE たいとるだよー"
	);

	g_pHintText = new FontRenderer(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 40.0f },
		28.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"Press Decide"
	);

	g_pNaiyo = new Sprite2D(
		{ 140.0f, 140.0f },
		{ 200.0f, 200.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_NONE,
		L"asset\\texture\\notfound_thumbnail.png"
	);
}

void Title_Update(void)
{
	// ===================== 入力操作について =====================
	//
	// コントローラー、キーボード双方の入力の実装をやりやすくするため、原則keyboard.hを直接読み取ることはしない
	// input_manager.hのInput_IsActionDown,Triggerを使用する。
	// document\input.mdを参照のこと。
	//
	// ==========================================================

	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE))
	{
		SetSceneFade(SCENE_DEBUG);
	}

	g_pTitleText->AddRot(360.0f * (1.0f / FPS / 4));
	g_pNaiyo->AddRot(-360.0f * (1.0f / FPS / 4));
}

void Title_Draw(void)
{
	if (g_pTitleText) g_pTitleText->Draw();
	if (g_pHintText) g_pHintText->Draw();
	if (g_pNaiyo) g_pNaiyo->Draw();
}

void Title_Finalize(void)
{
	SAFE_DELETE(g_pTitleText);
	SAFE_DELETE(g_pHintText);
	SAFE_DELETE(g_pNaiyo);
}
