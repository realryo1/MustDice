#include "shop.h"
#include "define.h"
#include "font.h"
#include "clickfont.h"
#include "input_manager.h"
#include "fade.h"

using namespace DirectX;

static Sprite2D* g_pNaiyo = nullptr;
static DrawFont* g_pShopText = nullptr;
static DrawFont* g_pHintText = nullptr;

void Shop_Initialize(void)
{
	g_pShopText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 40.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"Template"
	);

	g_pHintText = new DrawFont(
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

void Shop_Update(void)
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
		SetSceneFade(SCENE_RESULT);
	}

	//適当にぐるぐる
	g_pShopText->AddRot(360.0f * (1.0f / FPS / 4));
	g_pNaiyo->AddRot(-360.0f * (1.0f / FPS / 4));
}

void Shop_Draw(void)
{
	if (g_pShopText) g_pShopText->Draw();
	if (g_pHintText) g_pHintText->Draw();
	if (g_pNaiyo) g_pNaiyo->Draw();
}

void Shop_Finalize(void)
{
	SAFE_DELETE(g_pShopText);
	SAFE_DELETE(g_pHintText);
	SAFE_DELETE(g_pNaiyo);
}
