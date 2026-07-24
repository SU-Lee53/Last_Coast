#include "pch.h"
#include "GrenadeProjectile.h"
#include "Collider.h"

void GrenadeProjectile::Initialize()
{
	if (!GetComponent<Transform>()) {
		AddComponent<Transform>();
	}

	for (auto& component : m_pComponents) {
		if (component) {
			component->Initialize();
		}
	}

	auto pModel = MODEL->LoadOrGet("granade")->CopyObject<NodeObject>();
	pModel->GetTransform()->Scale(MODEL_SCALE);
	SetChild(pModel);

	GetTransform()->Update();

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	// 모델 바운드 기반 소형 캡슐 — MeshCollider(도로/건물 UCX) 삼각형 narrow-phase용
	AddComponent<PlayerCollider>();
	GetComponent<PlayerCollider>()->Initialize();

	m_bInitialized = true;
}

void GrenadeProjectile::Launch(const Vector3& position, const Vector3& velocity)
{
	auto pTransform = GetTransform();
	pTransform->SetPosition(position);
	// GetPosition()은 월드 행렬을 읽는다 — 즉시 갱신 안 하면 첫 Update 프레임이
	// 낡은 월드 행렬(원점)을 읽어 (0,0,0)으로 순간이동한다.
	pTransform->Update();

	if (auto pCollider = GetComponent<PlayerCollider>()) {
		pCollider->Update();	// 캡슐도 스폰 위치로 동기화
	}

	m_v3Velocity = velocity;
	m_fFuseTimer = 0.f;
	m_bExploded  = false;
}

namespace
{
	// 구 vs OBB 침투 해소 + 반사. 충돌 시 위치 밀어내고 속도 반사(반발 계수 적용).
	bool ResolveSphereOBB(const BoundingOrientedBox& obb, float radius, float restitution,
	                      Vector3& pos, Vector3& vel)
	{
		const Quaternion qRot{ obb.Orientation };
		Quaternion qInv = qRot;
		qInv.Conjugate();

		const Vector3 v3Local = Vector3::Transform(pos - Vector3{ obb.Center }, qInv);
		const Vector3 v3Ext{ obb.Extents };
		const Vector3 v3Closest{
			std::clamp(v3Local.x, -v3Ext.x, v3Ext.x),
			std::clamp(v3Local.y, -v3Ext.y, v3Ext.y),
			std::clamp(v3Local.z, -v3Ext.z, v3Ext.z) };

		Vector3 v3Delta = v3Local - v3Closest;
		const float fDistSq = v3Delta.LengthSquared();
		if (fDistSq > radius * radius) return false;

		// 중심이 OBB 내부면 무시 — 씬의 대형 충돌 박스(공중을 덮는 broad OBB) 안에서
		// 스폰/통과하는 경우가 있어, 내부 탈출을 시도하면 수십 미터 순간이동이 난다.
		// 표면 스침 접촉만 해소하면 위에서 떨어질 때 윗면 바운스는 정상 동작한다.
		if (fDistSq <= 1e-6f) return false;

		const float fDist = std::sqrt(fDistSq);
		const Vector3 v3NormalLocal = v3Delta / fDist;
		const float fPush = radius - fDist;

		const Vector3 v3Normal = Vector3::Transform(v3NormalLocal, qRot);
		pos += v3Normal * fPush;

		const float fVn = vel.Dot(v3Normal);
		if (fVn < 0.f) {
			vel -= v3Normal * fVn * (1.f + restitution);	// 반사 + 반발
		}
		return true;
	}
}

void GrenadeProjectile::Update()
{
	NodeObject::Update();	// 자식(모델) 업데이트 전파

	if (m_bDebugStatic || m_bExploded) {
		return;
	}

	m_fFuseTimer += DT;
	if (m_fFuseTimer >= FUSE_SECONDS) {
		m_bExploded = true;
		return;
	}

	StepPhysics(DT);
}

void GrenadeProjectile::StepPhysics(float deltaTime)
{
	m_v3Velocity.y += GRAVITY * deltaTime;

	auto pTransform = GetTransform();
	Vector3 v3Pos = pTransform->GetPosition() + m_v3Velocity * deltaTime;

	// 이동 반영 + 캡슐 월드 갱신 (충돌 판정 전 필요)
	auto pSelfCollider = GetComponent<PlayerCollider>();
	auto ApplyPos = [&]() {
		pTransform->SetPosition(v3Pos);
		pTransform->Update();
		if (pSelfCollider) pSelfCollider->Update();
	};
	ApplyPos();

	// ── 1. 정적 충돌 (도시 바닥/건물은 지형이 아니라 정적 메시라 필수) ─────────
	// MeshCollider(UCX)는 캡슐-삼각형 narrow-phase(플레이어와 동일 경로),
	// 그 외 박스 콜라이더는 구-OBB 표면 접촉만 해소.
	bool bBounced = false;
	{
		SpatialQueryDesc desc{};
		desc.unLayerMask = SPATIAL_COLLIDABLE | SPATIAL_STATIC;
		desc.eLayerMatchMode = SPATIAL_LAYER_MATCH_MODE::ALL;
		desc.bIncludeStatic = true;
		desc.bIncludeDynamic = false;

		const float fQueryExtent = BODY_RADIUS + 50.f;
		const BoundingBox queryBox{ v3Pos, Vector3{ fQueryExtent, fQueryExtent, fQueryExtent } };

		const auto hits = CUR_SCENE->GetWorld().GetSpatial().QueryAABB(queryBox, desc);
		for (auto* pObj : hits.pObjects) {
			if (!pObj) continue;
			auto pCollider = pObj->GetComponentFromRoot<ICollider>();
			if (!pCollider) continue;

			if (auto pMesh = std::dynamic_pointer_cast<MeshCollider>(pCollider)) {
				if (!pSelfCollider || !pMesh->CheckCollision(pSelfCollider)) continue;

				for (const auto& [v3Normal, fDepth] : pMesh->GetContacts()) {
					v3Pos += v3Normal * fDepth;
					const float fVn = m_v3Velocity.Dot(v3Normal);
					if (fVn < 0.f) {
						m_v3Velocity -= v3Normal * fVn * (1.f + RESTITUTION);	// 반사 + 반발
					}
					bBounced = true;
				}
				ApplyPos();	// 다음 콜라이더 판정 전 캡슐 위치 반영
			}
			else if (ResolveSphereOBB(pCollider->GetOBBWorld(), BODY_RADIUS, RESTITUTION, v3Pos, m_v3Velocity)) {
				bBounced = true;
				ApplyPos();
			}
		}
	}

	// ── 2. 지형 바운스 (야외 폴백) — 구 하단 기준으로 침투 판정 ───────────────
	Vector3 v3Probe = v3Pos;
	v3Probe.y -= BODY_RADIUS;
	TerrainHit hit = CUR_SCENE->QueryTerrainHit(v3Probe);
	if (hit.bGrounded) {
		v3Pos.y = hit.fHeight + BODY_RADIUS;
		if (m_v3Velocity.y < 0.f) {
			m_v3Velocity.y = -m_v3Velocity.y * RESTITUTION;
			bBounced = true;
		}
	}

	if (bBounced) {
		m_v3Velocity.x *= XZ_DAMPING;
		m_v3Velocity.z *= XZ_DAMPING;

		if (m_v3Velocity.Length() < REST_SPEED) {
			m_v3Velocity = Vector3::Zero;	// 정지 — 제자리에서 퓨즈만 진행
		}
	}

	ApplyPos();
}

Vector3 GrenadeProjectile::GetPosition() const
{
	return GetComponent<Transform>()->GetPosition();
}
