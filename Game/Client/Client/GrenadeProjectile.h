#pragma once
#include "NodeObject.h"

// 투척 수류탄 투사체 — 포물선 + 지형 바운스 + 퓨즈 타이머.
// 물리 시뮬은 각 클라이언트 로컬(동일 초기조건이라 근사 일치, 시각 전용).
// 폭발 처리(연출/데미지/제거)는 GameScene::UpdateGrenades가 IsExploded() 폴링으로 수행 —
// 던진 쪽만 데미지: 오프라인=씬 AoE, 온라인=C2S_GRENADE_EXPLODE 전송.
class GrenadeProjectile : public NodeObject {
public:
	void Initialize() override;
	void Update() override;

	// 초기 위치+속도로 발사 (Initialize 이후 호출)
	void Launch(const Vector3& position, const Vector3& velocity);

	void SetLocallyOwned(bool bOwned) { m_bLocallyOwned = bOwned; }
	bool IsLocallyOwned() const { return m_bLocallyOwned; }
	bool IsExploded() const { return m_bExploded; }
	Vector3 GetPosition() const;

	// 디버그: 물리/퓨즈 없이 제자리 표시 (모델 확인용)
	void SetDebugStatic(bool bStatic) { m_bDebugStatic = bStatic; }

public:
	static constexpr float FUSE_SECONDS   = 3.f;     // 투척 → 폭발까지
	static constexpr float EXPLODE_RADIUS = 700.f;   // AoE 반경 (cm) — 서버 GRENADE_RADIUS와 일치
	static constexpr float DAMAGE_MAX     = 150.f;   // 중심 데미지 (오프라인 판정용)
	static constexpr float DAMAGE_MIN     = 30.f;    // 반경 끝 데미지

private:
	// 중력 적분 + 정적 충돌(캡슐-삼각형/구-OBB) + 지형 폴백 한 스텝
	void StepPhysics(float deltaTime);

private:
	Vector3 m_v3Velocity    = Vector3::Zero;
	float   m_fFuseTimer    = 0.f;
	bool    m_bExploded     = false;
	bool    m_bLocallyOwned = false;
	bool    m_bDebugStatic  = false;

	static constexpr float GRAVITY     = -980.f;  // cm/s^2
	static constexpr float RESTITUTION = 0.45f;   // 바닥 반발 계수
	static constexpr float XZ_DAMPING  = 0.65f;   // 바운스 시 수평 감쇠
	static constexpr float REST_SPEED  = 60.f;    // 이하로 느려지면 정지 (cm/s)
	static constexpr float BODY_RADIUS = 10.f;    // 지형 판정용 반지름 (cm)
	static constexpr float MODEL_SCALE = 1.f;     // granade.bin 스케일 (모델 크기 보고 조정)
};
