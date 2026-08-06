#include "title.h"
#include "define.h"
#include "font.h"
#include "clickfont.h"
#include "input_manager.h"
#include "fade.h"
#include "run_session.h"

using namespace DirectX;

static Sprite2D* g_pNaiyo = nullptr;
static DrawFont* g_pTitleText = nullptr;
static DrawFont* g_pHintText = nullptr;
static ClickFont* g_pDebugButton = nullptr;

void Title_Initialize(void)
{
	g_pNaiyo = new Sprite2D(
		{ 140.0f, 140.0f },
		{ 200.0f, 200.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_NONE,
		L"asset\\texture\\notfound_thumbnail.png"
	);

	g_pTitleText = new DrawFont(
		{ SCREEN_X / 2.0f, SCREEN_Y / 2.0f - 40.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"MustDice"
	);

	g_pHintText = new DrawFont(
		{ SCREEN_X / 2.0f, SCREEN_Y / 2.0f + 40.0f },
		28.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"Press Enter/A"
	);

	g_pDebugButton = new ClickFont(
		{ SCREEN_X - 100.0f , 50.0f },
		20.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		{ 0.8f, 0.8f, 0.8f, 0.5f },
		"[debugscene]"
	);
}

void Title_Update(void)
{
	// ===================== 入力操作について =====================

	// コントローラー、キーボード双方の入力の実装をやりやすくするため、原則keyboard.hを直接読み取ることはしない
	// input_manager.hのInput_IsActionDown,Triggerを使用する。
	// document\input.mdを参照のこと。

	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE) && GetFadeState() == FADE_NONE)
	{
		//シーン遷移はSetSceneFadeのみ
		RunSession_Reset();
		SetSceneFade(SCENE_GAME);
	}

	// ==========================================================

	//適当にぐるぐる
	//g_pTitleText->AddRot(360.0f * (1.0f / FPS / 4));
	g_pNaiyo->AddRot(-360.0f * (1.0f / FPS / 4));

	//clickfontはupdateとisclick両方必要
	//Debugのみ計算
	#if defined(_DEBUG)
	g_pDebugButton->Update();
	if (g_pDebugButton->IsClick())SetSceneFade(SCENE_DEBUG);
	#endif
}

void Title_Draw(void)
{
	if (g_pNaiyo) g_pNaiyo->Draw();
	if (g_pTitleText) g_pTitleText->Draw();
	if (g_pHintText) g_pHintText->Draw();

	//Debugのみ描画
	#if defined(_DEBUG)
	if (g_pDebugButton) g_pDebugButton->Draw();
	#endif
}

void Title_Finalize(void)
{
	SAFE_DELETE(g_pNaiyo);
	SAFE_DELETE(g_pTitleText);
	SAFE_DELETE(g_pHintText);
	SAFE_DELETE(g_pDebugButton);
}
