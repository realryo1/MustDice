#include "title.h"
#include "define.h"
#include "font.h"
#include "MultiLineDrawFont.h"
#include "clickfont.h"
#include "input_manager.h"
#include "fade.h"
#include "run_session.h"
#include "keyboard.h"
#include "option_yml.h"
#include "net_client.h"
#include "match_session.h"
#include "scene.h"
#include <cstdio>
#include <string>

using namespace DirectX;

enum TitleMode
{
	TITLE_MODE_MENU = 0,
	TITLE_MODE_IP,
	TITLE_MODE_NAME,
};

static Sprite2D* g_pNaiyo = nullptr;
static DrawFont* g_pTitleText = nullptr;
static MultiLineDrawFont* g_pHintText = nullptr;
static ClickFont* g_pDebugButton = nullptr;
static int g_menuIndex = 0;
static TitleMode g_mode = TITLE_MODE_MENU;
static std::string g_ip;
static std::string g_name;
static int g_port = OPTION_DEFAULT_PORT;
static std::string g_status;

static void Title_RefreshHint(void)
{
	if (!g_pHintText)
	{
		return;
	}
	char buf[256] = {};
	if (g_mode == TITLE_MODE_MENU)
	{
		std::snprintf(
			buf,
			sizeof(buf),
			"%s ローカル   %s マルチ\n上下で選択 / Enter",
			(g_menuIndex == 0) ? ">" : " ",
			(g_menuIndex == 1) ? ">" : " "
		);
	}
	else if (g_mode == TITLE_MODE_IP)
	{
		std::snprintf(buf, sizeof(buf), "IP: %s_\n数字と. / Enter確定 / Esc戻る", g_ip.c_str());
	}
	else
	{
		std::snprintf(buf, sizeof(buf), "名前: %s_\n英数字 / Enter接続 / Esc戻る\n%s", g_name.c_str(), g_status.c_str());
	}
	g_pHintText->SetText(buf);
}

static void Title_TypeIp(void)
{
	if (Keyboard_IsKeyDownTrigger(KK_BACK) && !g_ip.empty())
	{
		g_ip.pop_back();
	}
	if (Keyboard_IsKeyDownTrigger(KK_OEMPERIOD) && g_ip.size() < 31)
	{
		g_ip += '.';
	}
	for (int d = 0; d <= 9; ++d)
	{
		if (Keyboard_IsKeyDownTrigger(static_cast<Keyboard_Keys>(KK_D0 + d)) && g_ip.size() < 31)
		{
			g_ip += static_cast<char>('0' + d);
		}
	}
}

static void Title_TypeName(void)
{
	if (Keyboard_IsKeyDownTrigger(KK_BACK) && !g_name.empty())
	{
		g_name.pop_back();
	}
	for (int i = 0; i < 26; ++i)
	{
		if (Keyboard_IsKeyDownTrigger(static_cast<Keyboard_Keys>(KK_A + i)) && g_name.size() < 12)
		{
			g_name += static_cast<char>('A' + i);
		}
	}
	for (int d = 0; d <= 9; ++d)
	{
		if (Keyboard_IsKeyDownTrigger(static_cast<Keyboard_Keys>(KK_D0 + d)) && g_name.size() < 12)
		{
			g_name += static_cast<char>('0' + d);
		}
	}
}

void Title_Initialize(void)
{
	g_menuIndex = 0;
	g_mode = TITLE_MODE_MENU;
	g_status.clear();
	g_port = OPTION_DEFAULT_PORT;
	OptionYml_Load(g_ip, g_port, g_name);
	if (g_name.empty())
	{
		g_name = "Player";
	}

	g_pNaiyo = new Sprite2D(
		{ 140.0f, 140.0f },
		{ 200.0f, 200.0f },
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		BLENDSTATE_NONE,
		L"asset\\texture\\notfound_thumbnail.png"
	);

	g_pTitleText = new DrawFont(
		{ SCREEN_X / 2.0f, SCREEN_Y / 2.0f - 80.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"MustDice"
	);

	g_pHintText = new MultiLineDrawFont(
		{ SCREEN_X / 2.0f, SCREEN_Y / 2.0f + 40.0f },
		24.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		""
	);

	g_pDebugButton = new ClickFont(
		{ SCREEN_X - 100.0f , 50.0f },
		20.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		{ 0.8f, 0.8f, 0.8f, 0.5f },
		"[debugscene]"
	);
	Title_RefreshHint();
}

void Title_Update(void)
{
	if (g_mode == TITLE_MODE_IP)
	{
		Title_TypeIp();
		if (Input_IsActionTrigger(INPUT_ACTION_BACK))
		{
			g_mode = TITLE_MODE_MENU;
		}
		else if (Input_IsActionTrigger(INPUT_ACTION_DECIDE) && !g_ip.empty() && GetFadeState() == FADE_NONE)
		{
			g_mode = TITLE_MODE_NAME;
		}
		Title_RefreshHint();
	}
	else if (g_mode == TITLE_MODE_NAME)
	{
		Title_TypeName();
		if (Input_IsActionTrigger(INPUT_ACTION_BACK))
		{
			g_mode = TITLE_MODE_IP;
			g_status.clear();
		}
		else if (Input_IsActionTrigger(INPUT_ACTION_DECIDE) && !g_name.empty() && GetFadeState() == FADE_NONE)
		{
			OptionYml_Save(g_ip, g_port, g_name);
			MatchSession_Reset();
			if (!NetClient_Connect(g_ip.c_str(), g_port))
			{
				g_status = NetClient_LastError();
			}
			else
			{
				char hello[64] = {};
				std::snprintf(hello, sizeof(hello), "HELLO %s", g_name.c_str());
				NetClient_Send(hello);
				SetSceneFade(SCENE_MULTILOBBY);
			}
		}
		Title_RefreshHint();
	}
	else
	{
		if (Input_IsActionTrigger(INPUT_ACTION_MENU_UP) || Input_IsActionTrigger(INPUT_ACTION_MENU_DOWN))
		{
			g_menuIndex = 1 - g_menuIndex;
		}
		if (Input_IsActionTrigger(INPUT_ACTION_DECIDE) && GetFadeState() == FADE_NONE)
		{
			if (g_menuIndex == 0)
			{
				RunSession_Reset();
				SetSceneFade(SCENE_GAME);
			}
			else
			{
				g_mode = TITLE_MODE_IP;
			}
		}
		Title_RefreshHint();
	}

	g_pNaiyo->AddRot(-360.0f * (1.0f / FPS / 4));

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
