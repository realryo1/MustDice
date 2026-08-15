#include "game.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"
#include "run_session.h"
#include "bet_logic.h"
#include "dice3d.h"
#include "camera.h"
#include "light.h"
#include "main.h"
#include "renderer.h"
#include <cstdio>
#include <string>

using namespace DirectX;

enum BetKind {
	BET_KIND_NONE = 0,
	BET_KIND_PINPOINT,
	BET_KIND_ODD_EVEN,
};

// GAMEシーンで使用するUIと3Dダイスの実体。
static DrawFont* g_pPhaseText = nullptr;
static DrawFont* g_pStatusText = nullptr;
static DrawFont* g_pDetailText = nullptr;
static DrawFont* g_pHintText = nullptr;
static Dice3D* g_pDice0 = nullptr;
static Dice3D* g_pDice1 = nullptr;

// ダイス演出用のカメラ、配置、ライティング設定。
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

// 現在のフェーズ、賭け内容、確定した出目を保持するGAMEシーンの状態。
static GamePhase g_phase = GAME_PHASE_ROUND_START;
static BetKind g_betKind = BET_KIND_NONE;
static int g_pickSum = BET_PINPOINT_CENTER;
static bool g_pickOdd = true;
static int g_die0 = 0;
static int g_die1 = 0;
static int g_rolledSum = 0;
static int g_lastScore = 0;
static bool g_rollStarted = false;
static unsigned int g_rollSerial = 0;
static XMFLOAT3 g_diceCameraPos = GAME_DICE_ROLL_CAMERA_POS;
static XMFLOAT3 g_diceCameraTarget = GAME_DICE_ROLL_CAMERA_TARGET;
static float g_diceCameraTransitionElapsed = 0.0f;
static char g_detailBuf[192] = {};

// フェーズごとの表示文字列をUIへ渡すための変換処理。
static const char* GamePhase_ToLabel(GamePhase phase)
{
	switch (phase)
	{
	case GAME_PHASE_ROUND_START: return "ROUND_START";
	case GAME_PHASE_BET_SELECT: return "BET_SELECT";
	case GAME_PHASE_PINPOINT_PICK: return "PINPOINT_PICK";
	case GAME_PHASE_ODD_EVEN_PICK: return "ODD_EVEN_PICK";
	case GAME_PHASE_ROLL: return "ROLL";
	case GAME_PHASE_RESOLVE: return "RESOLVE";
	case GAME_PHASE_GOAL_CHECK: return "GOAL_CHECK";
	default: return "UNKNOWN";
	}
}

static const char* GamePhase_ToHint(GamePhase phase)
{
	switch (phase)
	{
	case GAME_PHASE_ROUND_START:
		return "Enter,A: 賭け方選択へ";
	case GAME_PHASE_BET_SELECT:
		return "Enter,A: ピンポイント / BackSpace,B: 奇数偶数";
	case GAME_PHASE_PINPOINT_PICK:
		return "Left/Right: 予想変更 / Enter,A: 確定 / Esc,X: 戻る";
	case GAME_PHASE_ODD_EVEN_PICK:
		return "Enter,A: 奇数 / BackSpace,B: 偶数 / Esc,X: 戻る";
	case GAME_PHASE_ROLL:
		return g_rollStarted
			? "サイコロが止まるまでお待ちください"
			: "Enter,A: サイコロを振る / Esc,X: 戻る";
	case GAME_PHASE_RESOLVE:
		return "Enter,A: 次へ";
	case GAME_PHASE_GOAL_CHECK:
		return "Enter,A: 結果へ進む";
	default:
		return "Press Enter,A";
	}
}

static XMFLOAT3 Game_LerpPosition(const XMFLOAT3& from, const XMFLOAT3& to, float amount)
{
	return {
		from.x + (to.x - from.x) * amount,
		from.y + (to.y - from.y) * amount,
		from.z + (to.z - from.z) * amount
	};
}

// ダイスを振る視点と、出目を確認する見下ろし視点を管理するカメラ処理。
static void Game_ApplyDiceCamera(void)
{
	if (GetCamera())
	{
		GetCamera()->UpdateView(g_diceCameraPos, g_diceCameraTarget);
	}
	SetCameraPosition(g_diceCameraPos);
}

static void Game_ResetDiceCamera(void)
{
	g_diceCameraPos = GAME_DICE_ROLL_CAMERA_POS;
	g_diceCameraTarget = GAME_DICE_ROLL_CAMERA_TARGET;
	g_diceCameraTransitionElapsed = 0.0f;
	Game_ApplyDiceCamera();
}

static void Game_BeginResultCameraTransition(void)
{
	g_diceCameraTransitionElapsed = 0.0f;
}

static void Game_UpdateResultCamera(float deltaTime)
{
	const bool isTransitioning =
		g_diceCameraTransitionElapsed < GAME_DICE_CAMERA_TRANSITION_TIME;
	g_diceCameraTransitionElapsed += deltaTime;
	float amount = g_diceCameraTransitionElapsed / GAME_DICE_CAMERA_TRANSITION_TIME;
	if (amount > 1.0f)
	{
		amount = 1.0f;
	}

	const float smoothAmount = amount * amount * (3.0f - 2.0f * amount);
	g_diceCameraPos = Game_LerpPosition(
		GAME_DICE_ROLL_CAMERA_POS,
		GAME_DICE_RESULT_CAMERA_POS,
		smoothAmount
	);
	g_diceCameraTarget = Game_LerpPosition(
		GAME_DICE_ROLL_CAMERA_TARGET,
		GAME_DICE_RESULT_CAMERA_TARGET,
		smoothAmount
	);
	Game_ApplyDiceCamera();

	if (isTransitioning)
	{
		RequestRedraw();
	}
}

static void Game_RefreshUi(void)
{
	// ラン状態と現在の選択内容から、GAMEシーン内の各テキストを更新する。
	if (!g_pPhaseText || !g_pStatusText || !g_pDetailText || !g_pHintText)
	{
		return;
	}

	char status[160] = {};
	std::snprintf(
		status,
		sizeof(status),
		"[Round%d]   score %d / %d  |  bet %d/%d  |  money %d",
		RunSession_GetRoundIndex(),
		RunSession_GetRoundScore(),
		RunSession_GetTargetScore(),
		RunSession_GetBetCount(),
		RUN_MAX_BETS_PER_ROUND,
		RunSession_GetMoney()
	);

	g_pPhaseText->SetText(std::string("GAME / ") + GamePhase_ToLabel(g_phase));
	g_pStatusText->SetText(status);
	g_pDetailText->SetText(g_detailBuf);
	g_pHintText->SetText(GamePhase_ToHint(g_phase));
}

static void Game_BeginDiceRoll(void)
{
	// BetLogic_Roll2d6でゲーム上の2個の出目を先に確定する。
	// 3Dダイスはこの結果を目標面として演出し、着地時に再抽選は行わない。
	BetLogic_Roll2d6(&g_die0, &g_die1);
	g_rolledSum = g_die0 + g_die1;
	g_rollStarted = true;
	++g_rollSerial;

	// 毎回まったく同じ軌道に見えないよう、投射速度へ小さな周期変化を加える。
	const int variationIndex = static_cast<int>(g_rollSerial % 5) - 2;
	const float variation = static_cast<float>(variationIndex) * 0.08f;

	// 確定済みの各出目と、左右で異なる初速・角速度を3Dダイスへ渡す。
	if (g_pDice0)
	{
		g_pDice0->StartRoll(
			g_die0,
			{ 1.45f + variation, 4.65f + 0.08f * static_cast<float>(g_die0 % 3), 0.55f - variation },
			{
				(520.0f + 31.0f * g_die0) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(430.0f + 17.0f * variationIndex) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(630.0f - 23.0f * g_die0) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE
			}
		);
	}

	if (g_pDice1)
	{
		g_pDice1->StartRoll(
			g_die1,
			{ -1.40f + variation, 4.85f + 0.08f * static_cast<float>(g_die1 % 3), -0.48f - variation },
			{
				(-570.0f - 27.0f * g_die1) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(460.0f - 19.0f * variationIndex) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE,
				(-560.0f + 21.0f * g_die1) * GAME_DICE_INITIAL_ANGULAR_SPEED_RATE
			}
		);
	}

	std::snprintf(g_detailBuf, sizeof(g_detailBuf), "サイコロをロール中……");
	Game_RefreshUi();
}

static void Game_FinishDiceRoll(void)
{
	// 2個のダイスが停止した後、選択した賭け方に対応するスコアを確定する。
	g_rollStarted = false;

	if (g_betKind == BET_KIND_PINPOINT)
	{
		g_lastScore = BetLogic_ScorePinpoint(g_pickSum, g_rolledSum);
		const int diff = (g_pickSum > g_rolledSum) ? (g_pickSum - g_rolledSum) : (g_rolledSum - g_pickSum);
		std::snprintf(
			g_detailBuf,
			sizeof(g_detailBuf),
			"予想合計 = %d / 出目「%d+%d=%d」/ 基礎倍率:x%.1f - %.1f / %+d",
			g_pickSum,
			g_die0,
			g_die1,
			g_rolledSum,
			BetLogic_MultForSum(g_pickSum),
			BET_PINPOINT_STEP * static_cast<float>(diff),
			g_lastScore
		);
	}
	else
	{
		g_lastScore = BetLogic_ScoreOddEven(g_pickOdd, g_rolledSum);
		const bool hit = (BetLogic_IsOdd(g_rolledSum) == g_pickOdd);
		std::snprintf(
			g_detailBuf,
			sizeof(g_detailBuf),
			"%s / 出目「%d+%d=%d」 / 【%s】x%.1f / %+d",
			g_pickOdd ? "奇数を選択" : "偶数を選択",
			g_die0,
			g_die1,
			g_rolledSum,
			hit ? "HIT!!!" : "MISS……",
			BetLogic_OddEvenMult(hit),
			g_lastScore
		);
	}

	// 計算結果をラン共有状態へ反映し、出目確認用カメラと結果フェーズへ移る。
	RunSession_ApplyBetScore(g_lastScore);
	Game_BeginResultCameraTransition();
	g_phase = GAME_PHASE_RESOLVE;
	Game_RefreshUi();
}

static void Game_SetPhase(GamePhase phase)
{
	// フェーズを切り替え、入場時に必要な状態初期化と案内文の生成を行う。
	g_phase = phase;

	switch (phase)
	{
	case GAME_PHASE_ROUND_START:
		std::snprintf(
			g_detailBuf,
			sizeof(g_detailBuf),
			"目標スコア %d を目指せ（最大%d回賭け）",
			RunSession_GetTargetScore(),
			RUN_MAX_BETS_PER_ROUND
		);
		break;
	case GAME_PHASE_BET_SELECT:
		std::snprintf(g_detailBuf, sizeof(g_detailBuf), "賭け方を選ぶ");
		break;
	case GAME_PHASE_PINPOINT_PICK:
		std::snprintf(
			g_detailBuf,
			sizeof(g_detailBuf),
			"予想合計 = %d  (倍率 x%.1f)",
			g_pickSum,
			BetLogic_MultForSum(g_pickSum)
		);
		break;
	case GAME_PHASE_ODD_EVEN_PICK:
		std::snprintf(g_detailBuf, sizeof(g_detailBuf), "奇数か偶数かを選ぶ");
		break;
	case GAME_PHASE_ROLL:
		g_rollStarted = false;
		Game_ResetDiceCamera();
		g_die0 = 0;
		g_die1 = 0;
		g_rolledSum = 0;
		if (g_pDice0)
		{
			g_pDice0->Reset({ -GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f });
		}
		if (g_pDice1)
		{
			g_pDice1->Reset({ GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f });
		}

		if (g_betKind == BET_KIND_PINPOINT)
		{
			std::snprintf(g_detailBuf, sizeof(g_detailBuf), "ピンポイント 予想合計 = %d で振る", g_pickSum);
		}
		else
		{
			std::snprintf(
				g_detailBuf,
				sizeof(g_detailBuf),
				"%s で振る",
				g_pickOdd ? "奇数" : "偶数"
			);
		}
		break;
	case GAME_PHASE_GOAL_CHECK:
		if (RunSession_IsTargetMet())
		{
			std::snprintf(
				g_detailBuf,
				sizeof(g_detailBuf),
				"クリア！ score %d >= %d",
				RunSession_GetRoundScore(),
				RunSession_GetTargetScore()
			);
		}
		else
		{
			std::snprintf(
				g_detailBuf,
				sizeof(g_detailBuf),
				"失敗… score %d < %d",
				RunSession_GetRoundScore(),
				RunSession_GetTargetScore()
			);
		}
		break;
	default:
		break;
	}

	Game_RefreshUi();
}

void Game_Initialize(void)
{
	// GAMEシーン入場時にラウンド状態、カメラ、ダイス、UIをまとめて初期化する。
	// ラウンド開始は常にGAME入場時に行い、Shop側では進めない。
	RunSession_BeginRound();

	g_betKind = BET_KIND_NONE;
	g_pickSum = BET_PINPOINT_CENTER;
	g_pickOdd = true;
	g_die0 = 0;
	g_die1 = 0;
	g_rolledSum = 0;
	g_lastScore = 0;
	g_rollStarted = false;
	g_rollSerial = 0;
	g_detailBuf[0] = '\0';

	Camera_Initialize();
	Game_ResetDiceCamera();

	g_pDice0 = new Dice3D(
		{ -GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f },
		GAME_DICE_SIZE
	);
	g_pDice1 = new Dice3D(
		{ GAME_DICE_POS_X, GAME_DICE_POS_Y, 0.0f },
		GAME_DICE_SIZE
	);

	g_pPhaseText = new DrawFont(
		{ SCREEN_X / 2.0f, 55.0f },
		20.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"GAME"
	);

	g_pStatusText = new DrawFont(
		{ SCREEN_X / 2.0f, 125.0f },
		40.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		""
	);

	g_pDetailText = new DrawFont(
		{ SCREEN_X / 2.0f, 215.0f },
		30.0f,
		0.0f,
		{ 1.0f, 1.0f, 0.85f, 1.0f },
		""
	);

	g_pHintText = new DrawFont(
		{ SCREEN_X / 2.0f, SCREEN_Y - 55.0f },
		30.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		""
	);

	Game_SetPhase(GAME_PHASE_ROUND_START);
}

void Game_Update(void)
{
	// 共通アクションAPIから、このフレームで使用する入力を取得する。
	const bool decide = Input_IsActionTrigger(INPUT_ACTION_DECIDE);
	const bool cancel = Input_IsActionTrigger(INPUT_ACTION_CANCEL);
	const bool back = Input_IsActionTrigger(INPUT_ACTION_BACK);
	const bool left = Input_IsActionTrigger(INPUT_ACTION_MENU_LEFT);
	const bool right = Input_IsActionTrigger(INPUT_ACTION_MENU_RIGHT);

	// 現在のフェーズに応じて、選択操作、ダイス演出、シーン遷移を進行する。
	switch (g_phase)
	{
	case GAME_PHASE_ROUND_START:
		if (decide)
		{
			Game_SetPhase(GAME_PHASE_BET_SELECT);
		}
		break;

	case GAME_PHASE_BET_SELECT:
		if (decide)
		{
			g_betKind = BET_KIND_PINPOINT;
			g_pickSum = BET_PINPOINT_CENTER;
			Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
		}
		else if (cancel)
		{
			g_betKind = BET_KIND_ODD_EVEN;
			Game_SetPhase(GAME_PHASE_ODD_EVEN_PICK);
		}
		break;

	case GAME_PHASE_PINPOINT_PICK:
		if (back)
		{
			g_betKind = BET_KIND_NONE;
			Game_SetPhase(GAME_PHASE_BET_SELECT);
		}
		else if (left && g_pickSum > BET_SUM_MIN)
		{
			g_pickSum -= 1;
			Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
		}
		else if (right && g_pickSum < BET_SUM_MAX)
		{
			g_pickSum += 1;
			Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
		}
		else if (decide)
		{
			Game_SetPhase(GAME_PHASE_ROLL);
		}
		break;

	case GAME_PHASE_ODD_EVEN_PICK:
		if (back)
		{
			g_betKind = BET_KIND_NONE;
			Game_SetPhase(GAME_PHASE_BET_SELECT);
		}
		else if (decide)
		{
			g_pickOdd = true;
			Game_SetPhase(GAME_PHASE_ROLL);
		}
		else if (cancel)
		{
			g_pickOdd = false;
			Game_SetPhase(GAME_PHASE_ROLL);
		}
		break;

	case GAME_PHASE_ROLL:
		if (g_rollStarted)
		{
			// 2個のダイスを更新して衝突を解決し、両方の静止を待って結果を確定する。
			const float deltaTime = 1.0f / static_cast<float>(FPS);
			if (g_pDice0) g_pDice0->Update(deltaTime);
			if (g_pDice1) g_pDice1->Update(deltaTime);
			if (g_pDice0 && g_pDice1) g_pDice0->ResolveCollision(*g_pDice1);

			const bool dice0Settled = g_pDice0 && g_pDice0->IsSettled();
			const bool dice1Settled = g_pDice1 && g_pDice1->IsSettled();
			if (dice0Settled && dice1Settled)
			{
				Game_FinishDiceRoll();
			}
		}
		else if (back)
		{
			if (g_betKind == BET_KIND_PINPOINT)
			{
				Game_SetPhase(GAME_PHASE_PINPOINT_PICK);
			}
			else if (g_betKind == BET_KIND_ODD_EVEN)
			{
				Game_SetPhase(GAME_PHASE_ODD_EVEN_PICK);
			}
			else
			{
				Game_SetPhase(GAME_PHASE_BET_SELECT);
			}
		}
		else if (decide)
		{
			Game_BeginDiceRoll();
		}
		break;

	case GAME_PHASE_RESOLVE:
		// 出目を見やすいカメラへ移動し、決定入力後に次の賭けか目標判定へ進む。
		Game_UpdateResultCamera(1.0f / static_cast<float>(FPS));
		if (decide)
		{
			if (RunSession_IsBetLimitReached())
			{
				Game_SetPhase(GAME_PHASE_GOAL_CHECK);
			}
			else
			{
				g_betKind = BET_KIND_NONE;
				Game_SetPhase(GAME_PHASE_BET_SELECT);
			}
		}
		break;

	case GAME_PHASE_GOAL_CHECK:
		// ラウンド目標の達成結果に応じて、ショップまたはリザルトへ遷移する。
		if (decide && GetFadeState() == FADE_NONE)
		{
			if (RunSession_IsTargetMet())
			{
				RunSession_GrantClearReward();
				SetSceneFade(SCENE_SHOP);
			}
			else
			{
				SetSceneFade(SCENE_RESULT);
			}
		}
		break;

	default:
		break;
	}
}

void Game_Draw(void)
{
	// ダイス表示中は深度を有効にして3Dを描画し、その後2DのUIを重ねる。
	if (g_phase == GAME_PHASE_ROLL || g_phase == GAME_PHASE_RESOLVE)
	{
		SetDepthEnable(true);
		Game_ApplyDiceCamera();
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

void Game_Finalize(void)
{
	// GAMEシーンで生成したダイス、カメラ、UIリソースを解放する。
	SAFE_DELETE(g_pDice0);
	SAFE_DELETE(g_pDice1);
	Camera_Finalize();

	SAFE_DELETE(g_pPhaseText);
	SAFE_DELETE(g_pStatusText);
	SAFE_DELETE(g_pDetailText);
	SAFE_DELETE(g_pHintText);
}
