#include "dice3d.h"
#include "bet_logic.h"
#include "define.h"
#include "sprite3d.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

namespace
{
	// ダイスの落下、反発、停止、回転補正に使用する固定パラメータ。
	const char* DICE_MODEL_PATH = "asset\\model\\dice.fbx";
	const float DICE_GRAVITY = -11.0f;
	const float DICE_RESTITUTION = 0.34f;
	const float DICE_MIN_BOUNCE_IMPACT_SPEED = 0.75f;
	const float DICE_COLLISION_RESTITUTION = 0.42f;
	const float DICE_GROUND_LINEAR_DAMPING = 0.08f;
	const float DICE_ROTATION_PLAN_MIN_TIME = 0.85f;
	const float DICE_ROTATION_PLAN_MAX_TIME = 1.45f;
	const float DICE_CORRECTION_BOUNCE_SPEED = 1.35f;
	const int DICE_MAX_CORRECTION_BOUNCES = 2;
	const float DICE_MIN_ROLL_TIME = 1.55f;
	const float DICE_MAX_ROLL_TIME = 3.25f;
	const float DICE_ABSOLUTE_MAX_ROLL_TIME = 4.00f;
	const float DICE_SETTLE_DURATION = 0.12f;
	const float DICE_STOP_LINEAR_SPEED = 0.48f;
	const float DICE_STOP_ANGULAR_SPEED = 12.0f;
	const float DICE_STOP_ROTATION_ERROR = 2.5f;
	const float DICE_COLLISION_RADIUS_RATE = 0.55f;

	// 簡易物理と角度計算で共通使用する補助関数。
	float Length3(const XMFLOAT3& value)
	{
		return std::sqrt(
			value.x * value.x +
			value.y * value.y +
			value.z * value.z
		);
	}

	float ClampFloat(float value, float minimum, float maximum)
	{
		if (value < minimum)
		{
			return minimum;
		}
		if (value > maximum)
		{
			return maximum;
		}
		return value;
	}
}

// ダイスモデルを生成し、指定された位置と大きさで待機状態にする。
Dice3D::Dice3D(const XMFLOAT3& position, float size)
	: m_pModel(nullptr)
	, m_Position(position)
	, m_Rotation(0.0f, 0.0f, 0.0f)
	, m_Velocity(0.0f, 0.0f, 0.0f)
	, m_AngularVelocity(0.0f, 0.0f, 0.0f)
	, m_SettleStartRotation(0.0f, 0.0f, 0.0f)
	, m_TargetRotation(0.0f, 0.0f, 0.0f)
	, m_RotationPlanStart(0.0f, 0.0f, 0.0f)
	, m_RotationPlanTarget(0.0f, 0.0f, 0.0f)
	, m_RotationPlanInitialVelocity(0.0f, 0.0f, 0.0f)
	, m_Size(size)
	, m_FloorY(position.y - size * 0.5f)
	, m_RollElapsed(0.0f)
	, m_SettleElapsed(0.0f)
	, m_RotationPlanElapsed(0.0f)
	, m_RotationPlanDuration(0.0f)
	, m_TargetFace(1)
	, m_CorrectionBounceCount(0)
	, m_State(MOTION_IDLE)
{
	m_pModel = new Sprite3D(
		m_Position,
		{ m_Size, m_Size, m_Size },
		m_Rotation,
		DICE_MODEL_PATH,
		S_PHONG
	);
}

Dice3D::~Dice3D()
{
	SAFE_DELETE(m_pModel);
}

void Dice3D::Reset(const XMFLOAT3& position)
{
	// 前回のロール情報を破棄し、「1が上」の初期姿勢へ戻す。
	m_Position = position;
	m_Rotation = { 0.0f, 0.0f, 0.0f };
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	m_AngularVelocity = { 0.0f, 0.0f, 0.0f };
	m_SettleStartRotation = m_Rotation;
	m_TargetRotation = m_Rotation;
	m_RotationPlanStart = m_Rotation;
	m_RotationPlanTarget = m_Rotation;
	m_RotationPlanInitialVelocity = { 0.0f, 0.0f, 0.0f };
	m_FloorY = position.y - m_Size * 0.5f;
	m_RollElapsed = 0.0f;
	m_SettleElapsed = 0.0f;
	m_RotationPlanElapsed = 0.0f;
	m_RotationPlanDuration = 0.0f;
	m_TargetFace = 1;
	m_CorrectionBounceCount = 0;
	m_State = MOTION_IDLE;
	SyncModelTransform();
}

void Dice3D::StartRoll(
	int targetFace,
	const XMFLOAT3& launchVelocity,
	const XMFLOAT3& angularVelocity
)
{
	// 不正な出目が渡された場合は、サイコロとして有効な最小値へ補正する。
	if (targetFace < BET_DIE_FACE_MIN || targetFace > BET_DIE_FACE_MAX)
	{
		targetFace = BET_DIE_FACE_MIN;
	}

	m_TargetFace = targetFace;
	m_Velocity = launchVelocity;
	m_RollElapsed = 0.0f;
	m_SettleElapsed = 0.0f;
	m_CorrectionBounceCount = 0;

	// 予測した最終着地時刻を基準に、現在の回転速度から自然につながる
	// 目標姿勢と回転軌道をロール開始時点で組み立てる。
	const float rotationPlanDuration = CalculateRotationPlanDuration(launchVelocity.y);
	const XMFLOAT3 naturalLandingRotation = {
		m_Rotation.x + angularVelocity.x * rotationPlanDuration * 0.5f,
		m_Rotation.y + angularVelocity.y * rotationPlanDuration * 0.5f,
		m_Rotation.z + angularVelocity.z * rotationPlanDuration * 0.5f
	};
	m_TargetRotation = SelectNearestTargetRotation(
		m_TargetFace,
		naturalLandingRotation
	);
	BeginRotationPlan(angularVelocity, rotationPlanDuration, true);
	m_State = MOTION_ROLLING;
}

void Dice3D::Update(float deltaTime)
{
	if (deltaTime <= 0.0f)
	{
		return;
	}

	if (m_State == MOTION_ROLLING)
	{
		// 重力を加えながら、投射されたダイスの位置を更新する。
		m_RollElapsed += deltaTime;
		m_Velocity.y += DICE_GRAVITY * deltaTime;

		m_Position.x += m_Velocity.x * deltaTime;
		m_Position.y += m_Velocity.y * deltaTime;
		m_Position.z += m_Velocity.z * deltaTime;

		// 着地直前に目標面へ急回転しないよう、事前計画した軌道に沿って回転する。
		UpdateRotationPlan(deltaTime);

		// 床より下へ進まないよう位置を戻し、衝突速度に応じて反発または接地させる。
		const float restingY = m_FloorY + m_Size * 0.5f;
		bool isGrounded = false;
		if (m_Position.y <= restingY)
		{
			m_Position.y = restingY;
			isGrounded = true;

			if (m_Velocity.y < -DICE_MIN_BOUNCE_IMPACT_SPEED)
			{
				m_Velocity.y = -m_Velocity.y * DICE_RESTITUTION;
			}
			else
			{
				m_Velocity.y = 0.0f;
			}

			const float linearDamping = std::pow(DICE_GROUND_LINEAR_DAMPING, deltaTime);
			m_Velocity.x *= linearDamping;
			m_Velocity.z *= linearDamping;
		}

		// 移動速度、回転速度、目標姿勢との誤差から、静止へ移れる状態か判定する。
		const float horizontalSpeed = std::sqrt(
			m_Velocity.x * m_Velocity.x +
			m_Velocity.z * m_Velocity.z
		);
		const float angularSpeed = Length3(m_AngularVelocity);
		const float rotationError = GetRotationError(m_Rotation, m_TargetRotation);
		const bool rotationPlanFinished =
			m_RotationPlanElapsed >= m_RotationPlanDuration;
		const bool hasLowMotion =
			std::fabs(m_Velocity.y) <= 0.20f &&
			horizontalSpeed <= DICE_STOP_LINEAR_SPEED &&
			angularSpeed <= DICE_STOP_ANGULAR_SPEED;
		const bool canSettle =
			m_RollElapsed >= DICE_MIN_ROLL_TIME &&
			rotationPlanFinished &&
			isGrounded &&
			hasLowMotion &&
			rotationError <= DICE_STOP_ROTATION_ERROR;
		const bool shouldCorrectLanding =
			m_RollElapsed >= DICE_MIN_ROLL_TIME &&
			rotationPlanFinished &&
			isGrounded &&
			(hasLowMotion || m_RollElapsed >= DICE_MAX_ROLL_TIME) &&
			rotationError > DICE_STOP_ROTATION_ERROR &&
			m_CorrectionBounceCount < DICE_MAX_CORRECTION_BOUNCES;

		if (canSettle)
		{
			BeginSettling();
		}
		else if (shouldCorrectLanding)
		{
			BeginCorrectionBounce();
		}
		else if (
			m_RollElapsed >= DICE_ABSOLUTE_MAX_ROLL_TIME &&
			rotationPlanFinished &&
			isGrounded
		)
		{
			// 時間切れでも大角度の接地回転は行わない。
			// 誤差が残っていれば最後の小バウンド内で姿勢を合わせる。
			if (rotationError > DICE_STOP_ROTATION_ERROR)
			{
				BeginCorrectionBounce();
			}
			else
			{
				BeginSettling();
			}
		}
	}
	else if (m_State == MOTION_SETTLING)
	{
		// 停止直前に残った小さな角度差だけを短時間で滑らかに補間する。
		m_SettleElapsed += deltaTime;
		const float rawAmount = std::min(m_SettleElapsed / DICE_SETTLE_DURATION, 1.0f);
		const float smoothAmount = rawAmount * rawAmount * (3.0f - 2.0f * rawAmount);

		m_Rotation.x = LerpAngle(m_SettleStartRotation.x, m_TargetRotation.x, smoothAmount);
		m_Rotation.y = LerpAngle(m_SettleStartRotation.y, m_TargetRotation.y, smoothAmount);
		m_Rotation.z = LerpAngle(m_SettleStartRotation.z, m_TargetRotation.z, smoothAmount);

		if (rawAmount >= 1.0f)
		{
			m_Rotation = m_TargetRotation;
			m_State = MOTION_SETTLED;
		}
	}

	SyncModelTransform();
}

void Dice3D::ResolveCollision(Dice3D& other)
{
	// 両方が転がっている間だけ、ダイス同士の簡易衝突を計算する。
	if (this == &other || m_State != MOTION_ROLLING || other.m_State != MOTION_ROLLING)
	{
		return;
	}

	// 各ダイスを球として近似し、中心間距離から重なりを検出する。
	const float dx = other.m_Position.x - m_Position.x;
	const float dy = other.m_Position.y - m_Position.y;
	const float dz = other.m_Position.z - m_Position.z;
	const float distanceSquared = dx * dx + dy * dy + dz * dz;
	const float minimumDistance =
		m_Size * DICE_COLLISION_RADIUS_RATE +
		other.m_Size * DICE_COLLISION_RADIUS_RATE;

	if (distanceSquared >= minimumDistance * minimumDistance)
	{
		return;
	}

	float distance = std::sqrt(distanceSquared);
	float normalX = 1.0f;
	float normalY = 0.0f;
	float normalZ = 0.0f;
	if (distance > 0.0001f)
	{
		normalX = dx / distance;
		normalY = dy / distance;
		normalZ = dz / distance;
	}
	else
	{
		distance = 0.0f;
	}

	// 重なった距離を半分ずつ押し戻し、モデル同士がめり込まないようにする。
	const float correction = (minimumDistance - distance) * 0.5f;
	m_Position.x -= normalX * correction;
	m_Position.y -= normalY * correction;
	m_Position.z -= normalZ * correction;
	other.m_Position.x += normalX * correction;
	other.m_Position.y += normalY * correction;
	other.m_Position.z += normalZ * correction;

	// 接近中の場合だけ、衝突法線方向へ反発速度を加える。
	const float relativeX = other.m_Velocity.x - m_Velocity.x;
	const float relativeY = other.m_Velocity.y - m_Velocity.y;
	const float relativeZ = other.m_Velocity.z - m_Velocity.z;
	const float closingSpeed =
		relativeX * normalX +
		relativeY * normalY +
		relativeZ * normalZ;

	if (closingSpeed < 0.0f)
	{
		const float impulse = -(1.0f + DICE_COLLISION_RESTITUTION) * closingSpeed * 0.5f;
		m_Velocity.x -= normalX * impulse;
		m_Velocity.y -= normalY * impulse;
		m_Velocity.z -= normalZ * impulse;
		other.m_Velocity.x += normalX * impulse;
		other.m_Velocity.y += normalY * impulse;
		other.m_Velocity.z += normalZ * impulse;

	}

	const float restingY = m_FloorY + m_Size * 0.5f;
	const float otherRestingY = other.m_FloorY + other.m_Size * 0.5f;
	m_Position.y = (std::max)(m_Position.y, restingY);
	other.m_Position.y = (std::max)(other.m_Position.y, otherRestingY);
	SyncModelTransform();
	other.SyncModelTransform();
}

bool Dice3D::IsRolling(void) const
{
	return m_State == MOTION_ROLLING || m_State == MOTION_SETTLING;
}

bool Dice3D::IsSettled(void) const
{
	return m_State == MOTION_SETTLED;
}

void Dice3D::Draw(void)
{
	if (m_pModel)
	{
		m_pModel->Draw();
	}
}

XMFLOAT3 Dice3D::GetFaceRotation(int face)
{
	// 出目を上面へ向けるための基準姿勢。dice.fbxは +Y=1、-Z=2、+X=3 として扱う。
	switch (face)
	{
	case 1: return { 0.0f, 0.0f, 0.0f };
	case 2: return { 90.0f, 0.0f, 0.0f };
	case 3: return { 0.0f, 0.0f, 90.0f };
	case 4: return { 0.0f, 0.0f, -90.0f };
	case 5: return { -90.0f, 0.0f, 0.0f };
	case 6: return { 180.0f, 0.0f, 0.0f };
	default: return { 0.0f, 0.0f, 0.0f };
	}
}

XMFLOAT3 Dice3D::SelectNearestTargetRotation(int face, const XMFLOAT3& currentRotation)
{
	// 同じ出目でもY軸方向に4通りあるため、現在の姿勢に最も近い向きを選ぶ。
	const XMFLOAT3 baseRotation = GetFaceRotation(face);
	XMFLOAT3 nearestRotation = baseRotation;
	float nearestError = GetRotationError(currentRotation, nearestRotation);

	for (int direction = 1; direction < 4; ++direction)
	{
		XMFLOAT3 candidate = baseRotation;
		candidate.y = NormalizeAngle(baseRotation.y + 90.0f * static_cast<float>(direction));
		const float error = GetRotationError(currentRotation, candidate);
		if (error < nearestError)
		{
			nearestRotation = candidate;
			nearestError = error;
		}
	}

	return nearestRotation;
}

float Dice3D::NormalizeAngle(float angle)
{
	while (angle > 180.0f)
	{
		angle -= 360.0f;
	}
	while (angle < -180.0f)
	{
		angle += 360.0f;
	}
	return angle;
}

float Dice3D::LerpAngle(float from, float to, float amount)
{
	return NormalizeAngle(from + NormalizeAngle(to - from) * amount);
}

float Dice3D::SelectPlannedTargetAngle(
	float targetAngle,
	float currentAngle,
	float angularVelocity,
	float duration
)
{
	// 初速のまま減速した場合の回転量へ近い周回数を選び、急な方向転換を避ける。
	const float naturalTarget =
		currentAngle + angularVelocity * duration * 0.5f;
	float plannedTarget = targetAngle + 360.0f * std::round(
		(naturalTarget - targetAngle) / 360.0f
	);

	// 目標角度の丸めで回転方向が終盤に反転しないよう、
	// Hermite曲線が単調になるだけの回転量を確保する。
	const float minimumTravel = std::fabs(angularVelocity * duration) / 3.0f;
	float travel = plannedTarget - currentAngle;
	if (angularVelocity > 0.0f && travel < minimumTravel)
	{
		const float turns = std::ceil((minimumTravel - travel) / 360.0f);
		plannedTarget += 360.0f * turns;
	}
	else if (angularVelocity < 0.0f && travel > -minimumTravel)
	{
		const float turns = std::ceil((minimumTravel + travel) / 360.0f);
		plannedTarget -= 360.0f * turns;
	}

	return plannedTarget;
}

float Dice3D::GetRotationError(const XMFLOAT3& from, const XMFLOAT3& to)
{
	const float errorX = NormalizeAngle(to.x - from.x);
	const float errorY = NormalizeAngle(to.y - from.y);
	const float errorZ = NormalizeAngle(to.z - from.z);
	return std::sqrt(errorX * errorX + errorY * errorY + errorZ * errorZ);
}

float Dice3D::CalculateRotationPlanDuration(float launchVelocityY) const
{
	// 初回落下と、その後に反発速度が閾値以下になるまでの滞空時間を合計する。
	// この予測時間を、回転が目標姿勢へ到達する時間として使用する。
	const float gravity = -DICE_GRAVITY;
	const float restingY = m_FloorY + m_Size * 0.5f;
	const float height = (std::max)(m_Position.y - restingY, 0.0f);
	const float upwardSpeed = (std::max)(launchVelocityY, 0.0f);
	const float impactSpeed = std::sqrt(
		upwardSpeed * upwardSpeed + 2.0f * gravity * height
	);
	const float firstLandingTime =
		(upwardSpeed + impactSpeed) / gravity;
	float finalLandingTime = firstLandingTime;
	float bounceImpactSpeed = impactSpeed;
	while (bounceImpactSpeed > DICE_MIN_BOUNCE_IMPACT_SPEED)
	{
		const float reboundSpeed = bounceImpactSpeed * DICE_RESTITUTION;
		finalLandingTime += 2.0f * reboundSpeed / gravity;
		bounceImpactSpeed = reboundSpeed;
	}

	return ClampFloat(
		finalLandingTime,
		DICE_ROTATION_PLAN_MIN_TIME,
		DICE_ROTATION_PLAN_MAX_TIME
	);
}

void Dice3D::BeginRotationPlan(
	const XMFLOAT3& initialAngularVelocity,
	float duration,
	bool includeFullTurns
)
{
	// 現在姿勢と角速度を保存し、着地まで連続的に減速する回転軌道を準備する。
	m_RotationPlanStart = m_Rotation;
	m_RotationPlanInitialVelocity = initialAngularVelocity;
	m_RotationPlanElapsed = 0.0f;
	m_RotationPlanDuration = (std::max)(duration, 0.001f);

	if (includeFullTurns)
	{
		// 通常ロールでは、現在の回転方向を保てるよう必要な360度回転を含める。
		m_RotationPlanTarget = {
			SelectPlannedTargetAngle(
				m_TargetRotation.x,
				m_Rotation.x,
				initialAngularVelocity.x,
				m_RotationPlanDuration
			),
			SelectPlannedTargetAngle(
				m_TargetRotation.y,
				m_Rotation.y,
				initialAngularVelocity.y,
				m_RotationPlanDuration
			),
			SelectPlannedTargetAngle(
				m_TargetRotation.z,
				m_Rotation.z,
				initialAngularVelocity.z,
				m_RotationPlanDuration
			)
		};
	}
	else
	{
		// 補正バウンドでは周回を増やさず、目標姿勢までの最短角度だけ動かす。
		m_RotationPlanTarget = {
			m_Rotation.x + NormalizeAngle(m_TargetRotation.x - m_Rotation.x),
			m_Rotation.y + NormalizeAngle(m_TargetRotation.y - m_Rotation.y),
			m_Rotation.z + NormalizeAngle(m_TargetRotation.z - m_Rotation.z)
		};
	}

	m_AngularVelocity = initialAngularVelocity;
}

void Dice3D::UpdateRotationPlan(float deltaTime)
{
	// 始点の角速度を維持しつつ終点の角速度が0になるHermite曲線で補間する。
	// これにより、ロール終盤だけ目標面へ急回転する見え方を抑える。
	m_RotationPlanElapsed = (std::min)(
		m_RotationPlanElapsed + deltaTime,
		m_RotationPlanDuration
	);
	const float amount = ClampFloat(
		m_RotationPlanElapsed / m_RotationPlanDuration,
		0.0f,
		1.0f
	);
	const float amountSquared = amount * amount;
	const float amountCubed = amountSquared * amount;
	const float positionStart = 2.0f * amountCubed - 3.0f * amountSquared + 1.0f;
	const float velocityStart = amountCubed - 2.0f * amountSquared + amount;
	const float positionTarget = -2.0f * amountCubed + 3.0f * amountSquared;
	const float derivativeStart = 6.0f * amountSquared - 6.0f * amount;
	const float derivativeVelocity = 3.0f * amountSquared - 4.0f * amount + 1.0f;
	const float derivativeTarget = -6.0f * amountSquared + 6.0f * amount;

	// 同じ補間式をX/Y/Zの各回転軸へ適用する。
	auto updateAxis = [=](
		float start,
		float target,
		float initialVelocity,
		float* rotation,
		float* angularVelocity
	)
	{
		const float unwrappedRotation =
			positionStart * start +
			velocityStart * m_RotationPlanDuration * initialVelocity +
			positionTarget * target;
		*rotation = NormalizeAngle(unwrappedRotation);
		*angularVelocity = (
			derivativeStart * start +
			derivativeVelocity * m_RotationPlanDuration * initialVelocity +
			derivativeTarget * target
		) / m_RotationPlanDuration;
	};

	updateAxis(
		m_RotationPlanStart.x,
		m_RotationPlanTarget.x,
		m_RotationPlanInitialVelocity.x,
		&m_Rotation.x,
		&m_AngularVelocity.x
	);
	updateAxis(
		m_RotationPlanStart.y,
		m_RotationPlanTarget.y,
		m_RotationPlanInitialVelocity.y,
		&m_Rotation.y,
		&m_AngularVelocity.y
	);
	updateAxis(
		m_RotationPlanStart.z,
		m_RotationPlanTarget.z,
		m_RotationPlanInitialVelocity.z,
		&m_Rotation.z,
		&m_AngularVelocity.z
	);

	if (amount >= 1.0f)
	{
		m_Rotation = m_TargetRotation;
		m_AngularVelocity = { 0.0f, 0.0f, 0.0f };
	}
}

void Dice3D::BeginCorrectionBounce(void)
{
	// 着地時に許容値を超える姿勢誤差が残った場合、小さな再バウンド中に補正する。
	const float correctionDuration =
		2.0f * DICE_CORRECTION_BOUNCE_SPEED / -DICE_GRAVITY;
	m_Position.y = m_FloorY + m_Size * 0.5f;
	m_Velocity.y = DICE_CORRECTION_BOUNCE_SPEED;
	++m_CorrectionBounceCount;
	BeginRotationPlan(
		{ 0.0f, 0.0f, 0.0f },
		correctionDuration,
		false
	);
}

void Dice3D::BeginSettling(void)
{
	// 並進と回転を止め、最終姿勢へ短時間で補間する状態へ移行する。
	m_Position.y = m_FloorY + m_Size * 0.5f;
	m_Velocity = { 0.0f, 0.0f, 0.0f };
	m_AngularVelocity = { 0.0f, 0.0f, 0.0f };
	m_SettleStartRotation = m_Rotation;
	m_SettleElapsed = 0.0f;
	m_State = MOTION_SETTLING;
}

void Dice3D::SyncModelTransform(void)
{
	// 簡易物理で保持している位置と回転を描画モデルへ反映する。
	if (!m_pModel)
	{
		return;
	}

	m_pModel->SetPos(m_Position);
	m_pModel->SetRot(m_Rotation);
}
