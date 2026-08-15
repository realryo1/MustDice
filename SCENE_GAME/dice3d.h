#pragma once

#include <DirectXMath.h>

class Sprite3D;

// 完成済みのダイスモデルを、簡易物理演算で転がして指定された目へ着地させる。
class Dice3D
{
public:
	Dice3D(const DirectX::XMFLOAT3& position, float size);
	~Dice3D();

	Dice3D(const Dice3D&) = delete;
	Dice3D& operator=(const Dice3D&) = delete;

	// 初期位置へ戻し、画像と同じ「1が上」の姿勢にする。
	void Reset(const DirectX::XMFLOAT3& position);
	// BetLogicで確定したtargetFaceへ着地するロールを開始する。
	void StartRoll(
		int targetFace,
		const DirectX::XMFLOAT3& launchVelocity,
		const DirectX::XMFLOAT3& angularVelocity
	);
	void Update(float deltaTime);
	// 2個のダイス同士を球で近似して衝突させる。
	void ResolveCollision(Dice3D& other);

	bool IsRolling(void) const;
	bool IsSettled(void) const;
	void Draw(void);

private:
	enum MotionState
	{
		MOTION_IDLE = 0,
		MOTION_ROLLING,
		MOTION_SETTLING,
		MOTION_SETTLED,
	};

	static DirectX::XMFLOAT3 GetFaceRotation(int face);
	static DirectX::XMFLOAT3 SelectNearestTargetRotation(
		int face,
		const DirectX::XMFLOAT3& currentRotation
	);
	static float NormalizeAngle(float angle);
	static float LerpAngle(float from, float to, float amount);
	static float SelectPlannedTargetAngle(
		float targetAngle,
		float currentAngle,
		float angularVelocity,
		float duration
	);
	static float GetRotationError(
		const DirectX::XMFLOAT3& from,
		const DirectX::XMFLOAT3& to
	);

	float CalculateRotationPlanDuration(float launchVelocityY) const;
	void BeginRotationPlan(
		const DirectX::XMFLOAT3& initialAngularVelocity,
		float duration,
		bool includeFullTurns
	);
	void UpdateRotationPlan(float deltaTime);
	void BeginCorrectionBounce(void);
	void BeginSettling(void);
	void SyncModelTransform(void);

	Sprite3D* m_pModel;
	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_Rotation;
	DirectX::XMFLOAT3 m_Velocity;
	DirectX::XMFLOAT3 m_AngularVelocity;
	DirectX::XMFLOAT3 m_SettleStartRotation;
	DirectX::XMFLOAT3 m_TargetRotation;
	DirectX::XMFLOAT3 m_RotationPlanStart;
	DirectX::XMFLOAT3 m_RotationPlanTarget;
	DirectX::XMFLOAT3 m_RotationPlanInitialVelocity;
	float m_Size;
	float m_FloorY;
	float m_RollElapsed;
	float m_SettleElapsed;
	float m_RotationPlanElapsed;
	float m_RotationPlanDuration;
	int m_TargetFace;
	int m_CorrectionBounceCount;
	MotionState m_State;
};
