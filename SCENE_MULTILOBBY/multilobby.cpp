#include "multilobby.h"
#include "define.h"
#include "font.h"
#include "MultiLineDrawFont.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"
#include "net_client.h"
#include "match_session.h"
#include <cstdio>
#include <string>

using namespace DirectX;

static MultiLineDrawFont* g_pBody = nullptr;
static DrawFont* g_pTitle = nullptr;
static bool g_readySent = false;

void Multilobby_Initialize(void)
{
	g_readySent = false;
	g_pTitle = new DrawFont(
		{ SCREEN_X / 2.0f, 80.0f },
		40.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"MULTI LOBBY"
	);
	g_pBody = new MultiLineDrawFont(
		{ SCREEN_X / 2.0f, 280.0f },
		22.0f,
		0.0f,
		{ 0.9f, 0.9f, 0.9f, 1.0f },
		""
	);
}

void Multilobby_Update(void)
{
	MatchSession_PollNet();
	MatchSession* ms = MatchSession_Get();

	if (ms->lastErr != 0 && GetFadeState() == FADE_NONE)
	{
		NetClient_Close();
		SetSceneFade(SCENE_TITLE);
		return;
	}

	if (ms->inMatch && GetFadeState() == FADE_NONE)
	{
		SetSceneFade(SCENE_MULTIGAME);
		return;
	}

	if (Input_IsActionTrigger(INPUT_ACTION_BACK) && GetFadeState() == FADE_NONE)
	{
		NetClient_Close();
		SetSceneFade(SCENE_TITLE);
		return;
	}

	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE) && !g_readySent)
	{
		NetClient_Send("READY");
		g_readySent = true;
	}

	if (Input_IsActionTrigger(INPUT_ACTION_CANCEL) && ms->playerCount > 0 && ms->playerCount < MATCH_MAX_PLAYERS && ms->myId >= 0)
	{
		NetClient_Send("FILLBOTS");
	}

	char buf[512] = {};
	std::string names;
	for (int i = 0; i < ms->playerCount; ++i)
	{
		const bool ready = (ms->readyMask & (1 << i)) != 0;
		char row[64] = {};
		std::snprintf(row, sizeof(row), "%s%s %s\n", ready ? "[R] " : "[ ] ", ms->players[i].name, (i == ms->myId) ? "(you)" : "");
		names += row;
	}
	std::snprintf(
		buf,
		sizeof(buf),
		"%s\n人数 %d  待ち列 %d  queue=%d\nEnter: Ready / BackSpace: Botで4人に埋める / Esc: 切断\n%s",
		ms->queue ? "次の試合待ち" : "公開ロビー",
		ms->playerCount,
		ms->waitingBehind,
		ms->queue,
		names.c_str()
	);
	if (g_pBody) g_pBody->SetText(buf);
}

void Multilobby_Draw(void)
{
	if (g_pTitle) g_pTitle->Draw();
	if (g_pBody) g_pBody->Draw();
}

void Multilobby_Finalize(void)
{
	SAFE_DELETE(g_pTitle);
	SAFE_DELETE(g_pBody);
}
