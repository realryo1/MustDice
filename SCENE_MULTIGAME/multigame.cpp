#include "multigame.h"
#include "define.h"
#include "font.h"
#include "clickfont.h"
#include "input_manager.h"
#include "fade.h"

using namespace DirectX;

static Sprite2D* g_pNaiyo = nullptr;
static DrawFont* g_pMultigameText = nullptr;
static DrawFont* g_pHintText = nullptr;

void Multigame_Initialize(void)
{
	g_pMultigameText = new DrawFont(
		{ SCREEN_X / 2.0f, SCREEN_Y / 2.0f - 40.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"MULTIGAME"
	);

	g_pHintText = new DrawFont(
		{ SCREEN_X / 2.0f, SCREEN_Y / 2.0f + 40.0f },
		28.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"Press Enter/A"
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

void Multigame_Update(void)
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
		SetSceneFade(SCENE_MULTIGAME);
	}

	//適当にぐるぐる
	g_pMultigameText->AddRot(360.0f * (1.0f / FPS / 4));
	g_pNaiyo->AddRot(-360.0f * (1.0f / FPS / 4));
}

void Multigame_Draw(void)
{
	if (g_pMultigameText) g_pMultigameText->Draw();
	if (g_pHintText) g_pHintText->Draw();
	if (g_pNaiyo) g_pNaiyo->Draw();
}

void Multigame_Finalize(void)
{
	SAFE_DELETE(g_pMultigameText);
	SAFE_DELETE(g_pHintText);
	SAFE_DELETE(g_pNaiyo);
}
