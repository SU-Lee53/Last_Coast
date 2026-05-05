#include "pch.h"
#include "StaticObject.h"
#include "Collider.h"
#include "BulletImpactEffect.h"

void StaticObject::Initialize()
{
	if (!m_bInitialized) {
		// 필수 Component(Transform) 우선 생성
		if (!GetComponent<Transform>()) {
			AddComponent<Transform>();
		}

		for (auto& component : m_pComponents) {
			if (component) {
				component->Initialize();
			}
		}

		GetTransform()->Update();

		m_bInitialized = true;
	}

	for (auto& pChild : m_pChildren) {
		pChild->Initialize();
	}

	if (m_pParent.expired() && !GetComponent<ICollider>()) {
		// Collider 의 경우 계층 변환의 자식 전파가 우선 필요하므로 마지막에 추가하고 Initialize
		AddComponent<StaticCollider>();
		GetComponent<StaticCollider>()->Initialize();
	}
}

void StaticObject::ProcessInput()
{
}

void StaticObject::PreUpdate()
{
}

void StaticObject::Update()
{
	for (const auto& pChild : m_pChildren) {
		pChild->Update();
	}
}

void StaticObject::PostUpdate()
{
	for (auto& component : m_pComponents | std::views::drop(std::to_underlying(COMPONENT_TYPE::TRANSFORM) + 1)) {
		if (component) {
			component->Update();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->PostUpdate();
	}
}

void StaticObject::OnTraceHit(const RayTraceHitResult& hitResult)
{
	ParticleEffectSpawnDesc desc{};
	desc.v3Position = hitResult.v3ImpactPoint;

	// 먼지는 표면 normal 방향으로 튀는 게 자연스러움
	desc.v3Direction = hitResult.v3ImpactNormal;
	desc.v3Direction.Normalize();

	desc.v3Normal = hitResult.v3ImpactNormal;
	desc.mtxWorld = Matrix::CreateWorld(
		desc.v3Position,
		desc.v3Direction,
		Vector3::Up
	);

	PARTICLE->Spawn<BulletImpactEffect>(desc);
}
