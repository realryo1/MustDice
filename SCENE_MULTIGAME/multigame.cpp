#include "multigame.h"
#include "define.h"
#include "font.h"
#include "MultiLineDrawFont.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"
#include "bet_logic.h"
#include "dice3d.h"
#include "camera.h"
#include "light.h"
#include "main.h"
#include "renderer.h"
#include "net_client.h"
#include "match_session.h"
#include <cstdio>
#include <string>

using namespace DirectX;

enum MultiPhase
{
	MULTI_PHASE_WAIT = 0,
	MULTI_PHASE_BET_SELECT,
	MULTI_PHASE_PINPOINT,
	MULTI_PHASE_ODD_EVEN,
	MULTI_PHASE_WAIT_RESOLVE,
	MULTI_PHASE_ROLL,
	MULTI_PHASE_SHOW,
	MULTI_PHASE_ROUND,
};

enum BetKind
{
	BET_KIND_NONE = 0,
	BET_KIND_PINPOINT,
	BET_KIND_ODD_EVEN,
};

static DrawFont* g_pPhaseText = nullptr;
static DrawFont* g_pStatusText = nullptr;
static MultiLineDrawFont* g_pDetailText = nullptr;
static DrawFont* g_pHintText = nullptr;
static Dice3D* g_pDice0 = nullptr;
static Dice3D* g_pDice1 = nullptr;

static const XMFLOAT3 GAME_DICE_ROLL_CAMERA_POS = { 0.0f, 1.2f, -8.0f };
static const XMFLOAT3 GAME_DICE_ROLL_CAMERA_TARGET = { 0.0f, 0.0f, 0.0f };
static const XMFLOAT3 GAME_DICE_RESULT_CAMERA_POS = { 0.0f, 5.5f, -3.2f };
static const XMFLOAT3 GAME_DICE_RESULT_CAMERA_TARGET = { 0.0f, -0.75f, 0.0f };
static const float GAME_DICE_CAMERA_TRANSITION_TIME = 0.6f;
static const float GAME_DICE_SIZE = 1.4f;
static const float GAME_DICE_POS_X = 1.0f;
static const float GAME_DICE_POS_Y = -0.75f;
static const float GAME_DICE_INITIAL_ANGULAR_SPEED_RATE = 1.35f;
static AmbientLight g_diceAmbientLight({ 0.35f, 0.35f, 0.35f, 1.0f });
static PointLight g_dicePointLight(
	TRUE,
	{ 0.0f, 4.0f, -5.0f, 1.0f },
	{ 1.0f, 1.0f, 1.0f, 1.0f },
	20.0f,
	1.2f
);

static MultiPhase g_phase = MULTI_PHASE_WAIT;
static BetKind g_betKind = BET_KIND_NONE;
static int g_pickSum = BET_PINPOINT_CENTER;
static bool g_pickOdd = true;
static bool g_rollStarted = false;
static unsigned int g_rollSerial = 0;
static int g_seenRound = 0;
static int g_seenBet = 0;
static int g_seenDie0 = -1;
static int g_seenDie1 = -1;
static bool g_betInputArmed = false;
static XMFLOAT3 g_diceCameraPos = GAME_DICE_ROLL_CAMERA_POS;
static XMFLOAT3 g_diceCameraTarget = GAME_DICE_ROLL_CAMERA_TARGET;
static float g_diceCameraTransitionElapsed = 0.0f;

static XMFLOAT3 Multi_Lerp(const XMFLOAT3& from, const XMFLOAT3& to, float amount)
{
	return {
		from.x + (to.x - from.x) * amount,
		from.y + (to.y - from.y) * amount,
		from.z + (to.z - from.z) * amount
	};
}

static void Multi_ApplyDiceCamera(void)
{
	if (GetCamera())
	{
		GetCamera()->UpdateView(g_diceCameraPos, g_diceCameraTarget);
	}
	SetCameraPosition(g_diceCameraPos);
}

static void Multi_ResetDiceCamera(void)
{
	g_diceCameraPos = GAME_DICE_ROLL_CAMERA_POS;
	g_diceCameraTarget = GAME_DICE_ROLL_CAMERA_TARGET;
	g_diceCameraTransitionElapsed = 0.0f;
	Multi_ApplyDiceCamera();
}

static void Multi_UpdateResultCamera(float deltaTime)
{
	g_diceCameraTransitionElapsed += deltaTime;
	float amount = g_diceCameraTransitionElapsed / GAME_DICE_CAMERA_TRANSITION_TIME;
	if (amount > 1.0f) amount = 1.0f;
	const float smoothAmount = amount * amount * (3.0f - 2.0f * amount);
	g_diceCameraPos = Multi_Lerp(GAME_DICE_ROLL_CAMERA_POS, GAME_DICE_RESULT_CAMERA_POS, smoothAmount);
	g_diceCameraTarget = Multi_Lerp(GAME_DICE_ROLL_CAMERA_TARGET, GAME_DICE_RESULT_CAMERA_TARGET, smoothAmount);
	Multi_ApplyDiceCamera();
	RequestRedraw();
}

static void Multi_BeginDiceRoll(int die0, int die1)
{
	g_rollStarted = true;
	++g_rollSerial;
	const int variationIndex = static_cast<int>(g_rollSerial % 5) - 2;
	const float variation = static_cast<float>(variationIndex) * 0.08f;
	if (g_pDice0)
	{
		g_pDice0->Reset({ -GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f });
		g_pDice0->StartRoll(
			die0,
			{ 1.45f + variation, 4.65f + 0.08f * static_cast<float>(die0 % 3), 0.55f - variation },
			{
				(520.0f + 31.0f * die0) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(430.0f + 17.0f * variationIndex) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(630.0f - 23.0f * die0) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE
			}
		);
	}
	if (g_pDice1)
	{
		g_pDice1->Reset({ GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f });
		g_pDice1->StartRoll(
			die1,
			{ -1.40f + variation, 4.85f + 0.08f * static_cast<float>(die1 % 3), -0.48f - variation },
			{
				(-570.0f - 27.0f * die1) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(460.0f - 19.0f * variationIndex) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(-560.0f + 21.0f * die1) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE
			}
		);
	}
	g_phase = MULTI_PHASE_ROLL;
	Multi_ResetDiceCamera();
}

static void Multi_RefreshUi(void)
{
	MatchSession* ms = MatchSession_Get();
	const int me = ms->myId;
	int myScore = 0;
	int myPts = 0;
	if (me >= 0 && me < MATCH_MAX_PLAYERS)
	{
		myScore = ms->players[me].roundScore;
		myPts = ms->players[me].matchPointX100;
	}
	char status[192] = {};
	std::snprintf(
		status,
		sizeof(status),
		"R%d Bet%d/%d  score %d  pt %.2f  rest %ds",
		ms->currentRound,
		ms->currentBet,
		3,
		myScore,
		myPts / 100.0f,
		ms->remainSec
	);
	if (g_pStatusText) g_pStatusText->SetText(status);

	const char* hint = "待機中";
	char detail[256] = {};
	switch (g_phase)
	{
	case MULTI_PHASE_WAIT:
		std::snprintf(detail, sizeof(detail), "サーバーの開始待ち");
		hint = "待機";
		break;
	case MULTI_PHASE_BET_SELECT:
		std::snprintf(detail, sizeof(detail), "賭け方を選ぶ");
		hint = "Enter: ピンポイント / BackSpace: 奇数偶数";
		break;
	case MULTI_PHASE_PINPOINT:
		std::snprintf(detail, sizeof(detail), "予想合計 = %d  (x%.1f)", g_pickSum, BetLogic_MultForSum(g_pickSum));
		hint = "Left/Right: 変更 / Enter: 送信 / Esc: 戻る";
		break;
	case MULTI_PHASE_ODD_EVEN:
		std::snprintf(detail, sizeof(detail), "奇数か偶数か");
		hint = "Enter: 奇数 / BackSpace: 偶数 / Esc: 戻る";
		break;
	case MULTI_PHASE_WAIT_RESOLVE:
		std::snprintf(detail, sizeof(detail), "提出済み。他プレイヤー待ち");
		hint = "待機";
		break;
	case MULTI_PHASE_ROLL:
		std::snprintf(detail, sizeof(detail), "ロール中");
		hint = "停止待ち";
		break;
	case MULTI_PHASE_SHOW:
		if (me >= 0)
		{
			std::snprintf(
				detail,
				sizeof(detail),
				"出目 %d+%d  今回 %+d  ラウンド %d",
				ms->players[me].die0,
				ms->players[me].die1,
				ms->players[me].lastScore,
				ms->players[me].roundScore
			);
		}
		hint = "次のベットを待機";
		break;
	case MULTI_PHASE_ROUND:
		{
			std::string board;
			for (int i = 0; i < ms->playerCount; ++i)
			{
				char row[80] = {};
				std::snprintf(
					row,
					sizeof(row),
					"%d位 %s score %d pt %.2f\n",
					ms->players[i].lastRank,
					ms->players[i].name,
					ms->players[i].roundScore,
					ms->players[i].matchPointX100 / 100.0f
				);
				board += row;
			}
			std::snprintf(detail, sizeof(detail), "%s", board.c_str());
			hint = "次ラウンド待ち";
		}
		break;
	default:
		break;
	}
	if (g_pPhaseText) g_pPhaseText->SetText("MULTIGAME");
	if (g_pDetailText) g_pDetailText->SetText(detail);
	if (g_pHintText) g_pHintText->SetText(hint);
}

void Multigame_Initialize(void)
{
	g_phase = MULTI_PHASE_WAIT;
	g_betKind = BET_KIND_NONE;
	g_pickSum = BET_PINPOINT_CENTER;
	g_pickOdd = true;
	g_rollStarted = false;
	g_seenRound = 0;
	g_seenBet = 0;
	g_seenDie0 = -1;
	g_seenDie1 = -1;
	g_betInputArmed = false;

	Camera_Initialize();
	Multi_ResetDiceCamera();
	g_pDice0 = new Dice3D({ -GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f }, GAME_DICE_SIZE);
	g_pDice1 = new Dice3D({ GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f }, GAME_DICE_SIZE);
	g_pPhaseText = new DrawFont({ SCREEN_X / 2.0f, 50.0f }, 20.0f, 0.0f, { 1,1,1,1 }, "MULTIGAME");
	g_pStatusText = new DrawFont({ SCREEN_X / 2.0f, 110.0f }, 28.0f, 0.0f, { 1,1,1,1 }, "");
	g_pDetailText = new MultiLineDrawFont({ SCREEN_X / 2.0f, 200.0f }, 24.0f, 0.0f, { 1.0f, 1.0f, 0.85f, 1.0f }, "");
	g_pHintText = new DrawFont({ SCREEN_X / 2.0f, SCREEN_Y - 50.0f }, 22.0f, 0.0f, { 1,1,1,1 }, "");
	Multi_RefreshUi();
}

static void Multi_Submit(void)
{
	char line[32] = {};
	if (g_betKind == BET_KIND_PINPOINT)
	{
		std::snprintf(line, sizeof(line), "BET P %d", g_pickSum);
	}
	else
	{
		std::snprintf(line, sizeof(line), "BET O %d", g_pickOdd ? 1 : 0);
	}
	NetClient_Send(line);
	g_phase = MULTI_PHASE_WAIT_RESOLVE;
}

void Multigame_Update(void)
{
	MatchSession_PollNet();
	MatchSession* ms = MatchSession_Get();

	if (!NetClient_IsConnected())
	{
		if (GetFadeState() == FADE_NONE)
		{
			SetSceneFade(SCENE_TITLE);
		}
		return;
	}
	if (ms->matchEnded && GetFadeState() == FADE_NONE)
	{
		SetSceneFade(SCENE_MULTIRESULT);
		return;
	}

	if (ms->currentRound != g_seenRound || ms->currentBet != g_seenBet)
	{
		if (ms->currentRound > 0 && ms->currentBet > 0)
		{
			g_seenRound = ms->currentRound;
			g_seenBet = ms->currentBet;
			g_betKind = BET_KIND_NONE;
			g_pickSum = BET_PINPOINT_CENTER;
			g_phase = MULTI_PHASE_BET_SELECT;
			g_rollStarted = false;
			g_seenDie0 = -1;
			g_seenDie1 = -1;
			g_betInputArmed = false;
			Multi_ResetDiceCamera();
		}
	}

	const int me = ms->myId;
	if (me >= 0 && me < MATCH_MAX_PLAYERS)
	{
		const int d0 = ms->players[me].die0;
		const int d1 = ms->players[me].die1;
		if (d0 > 0 && d1 > 0 && (d0 != g_seenDie0 || d1 != g_seenDie1) && !g_rollStarted)
		{
			g_seenDie0 = d0;
			g_seenDie1 = d1;
			Multi_BeginDiceRoll(d0, d1);
		}
	}

	if (!g_betInputArmed)
	{
		if (!Input_IsActionDown(INPUT_ACTION_DECIDE) && !Input_IsActionDown(INPUT_ACTION_CANCEL))
		{
			g_betInputArmed = true;
		}
	}

	const bool decide = g_betInputArmed && Input_IsActionTrigger(INPUT_ACTION_DECIDE);
	const bool cancel = g_betInputArmed && Input_IsActionTrigger(INPUT_ACTION_CANCEL);
	const bool back = Input_IsActionTrigger(INPUT_ACTION_BACK);
	const bool left = Input_IsActionTrigger(INPUT_ACTION_MENU_LEFT);
	const bool right = Input_IsActionTrigger(INPUT_ACTION_MENU_RIGHT);

	switch (g_phase)
	{
	case MULTI_PHASE_BET_SELECT:
		if (decide)
		{
			g_betKind = BET_KIND_PINPOINT;
			g_phase = MULTI_PHASE_PINPOINT;
		}
		else if (cancel)
		{
			g_betKind = BET_KIND_ODD_EVEN;
			g_phase = MULTI_PHASE_ODD_EVEN;
		}
		break;
	case MULTI_PHASE_PINPOINT:
		if (back)
		{
			g_phase = MULTI_PHASE_BET_SELECT;
		}
		else if (left && g_pickSum > BET_SUM_MIN)
		{
			g_pickSum -= 1;
		}
		else if (right && g_pickSum < BET_SUM_MAX)
		{
			g_pickSum += 1;
		}
		else if (decide)
		{
			Multi_Submit();
		}
		break;
	case MULTI_PHASE_ODD_EVEN:
		if (back)
		{
			g_phase = MULTI_PHASE_BET_SELECT;
		}
		else if (decide)
		{
			g_pickOdd = true;
			Multi_Submit();
		}
		else if (cancel)
		{
			g_pickOdd = false;
			Multi_Submit();
		}
		break;
	case MULTI_PHASE_ROLL:
		{
			const float deltaTime = 1.0f / static_cast<float>(FPS);
			if (g_pDice0) g_pDice0->Update(deltaTime);
			if (g_pDice1) g_pDice1->Update(deltaTime);
			if (g_pDice0 && g_pDice1) g_pDice0->ResolveCollision(*g_pDice1);
			if (g_pDice0 && g_pDice1 && g_pDice0->IsSettled() && g_pDice1->IsSettled())
			{
				g_rollStarted = false;
				g_diceCameraTransitionElapsed = 0.0f;
				g_phase = MULTI_PHASE_SHOW;
			}
		}
		break;
	case MULTI_PHASE_SHOW:
		Multi_UpdateResultCamera(1.0f / static_cast<float>(FPS));
		if (ms->roundBoard)
		{
			g_phase = MULTI_PHASE_ROUND;
		}
		break;
	default:
		break;
	}

	Multi_RefreshUi();
}

void Multigame_Draw(void)
{
	if (g_phase == MULTI_PHASE_ROLL || g_phase == MULTI_PHASE_SHOW)
	{
		SetDepthEnable(true);
		Multi_ApplyDiceCamera();
		g_dicePointLight.Apply(g_diceAmbientLight);
		if (g_pDice0) g_pDice0->Draw();
		if (g_pDice1) g_pDice1->Draw();
	}
	SetDepthEnable(false);
	if (g_pPhaseText) g_pPhaseText->Draw();
	if (g_pStatusText) g_pStatusText->Draw();
	if (g_pDetailText) g_pDetailText->Draw();
	if (g_pHintText) g_pHintText->Draw();
}

void Multigame_Finalize(void)
{
	SAFE_DELETE(g_pDice0);
	SAFE_DELETE(g_pDice1);
	Camera_Finalize();
	SAFE_DELETE(g_pPhaseText);
	SAFE_DELETE(g_pStatusText);
	SAFE_DELETE(g_pDetailText);
	SAFE_DELETE(g_pHintText);
}
