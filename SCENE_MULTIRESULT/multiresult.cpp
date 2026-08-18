#include "multiresult.h"
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

static DrawFont* g_pTitle = nullptr;
static MultiLineDrawFont* g_pBody = nullptr;

void Multiresult_Initialize(void)
{
	g_pTitle = new DrawFont(
		{ SCREEN_X / 2.0f, 80.0f },
		40.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"MATCH RESULT"
	);
	g_pBody = new MultiLineDrawFont(
		{ SCREEN_X / 2.0f, 280.0f },
		24.0f,
		0.0f,
		{ 0.95f, 0.95f, 0.95f, 1.0f },
		""
	);
}

void Multiresult_Update(void)
{
	MatchSession_PollNet();
	MatchSession* ms = MatchSession_Get();
	std::string text;
	for (int i = 0; i < ms->playerCount; ++i)
	{
		char row[128] = {};
		std::snprintf(
			row,
			sizeof(row),
			"%d位  %s  pt %.2f  1位回数 %d  累計スコア %d\n",
			ms->players[i].lastRank,
			ms->players[i].name,
			ms->players[i].matchPointX100 / 100.0f,
			ms->players[i].firstPlaceCount,
			ms->players[i].totalRoundScore
		);
		text += row;
	}
	text += "Enter: タイトルへ";
	if (g_pBody) g_pBody->SetText(text);

	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE) && GetFadeState() == FADE_NONE)
	{
		NetClient_Close();
		SetSceneFade(SCENE_TITLE);
	}
}

void Multiresult_Draw(void)
{
	if (g_pTitle) g_pTitle->Draw();
	if (g_pBody) g_pBody->Draw();
}

void Multiresult_Finalize(void)
{
	SAFE_DELETE(g_pTitle);
	SAFE_DELETE(g_pBody);
}
