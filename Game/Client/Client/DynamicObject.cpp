#include "pch.h"
#include "DynamicObject.h"
#include "TerrainObject.h"

void DynamicObject::PreUpdate()
{
}

void DynamicObject::PostUpdate()
{
	//ResolveTerrain();

	for (auto& component : m_pComponents) {
		if (component) {
			component->Update();
		}
	}

	for (auto& pChild : m_pChildren) {
		pChild->PostUpdate();
	}
}

void DynamicObject::ResolveTerrain(OUT Vector3& outv3Delta, OUT TerrainHit& outHitResult, bool bWasGrounded)
{
	auto& pTerrain = CUR_SCENE->GetTerrain();
	if (!pTerrain) {
		return;
	}

	// === Tunable Constants ===
	const float fSkin = 0.05f;

	// Slope Hysterisis
	const float fEnterGroundDot = 0.6f;
	const float fLeaveGroundDot = 0.55f;

	const float fSnapOnAir = 0.3f;
	const float fMaxSnapCap = 30.f;


	Vector3 v3Floor = Vector3::Transform(m_v3FloorPosition, GetWorldMatrix());
	Vector3 v3SampleCur = Vector3(v3Floor.x, 0.f, v3Floor.z);
	Vector3 v3SampleNext = Vector3(v3Floor.x + outv3Delta.x, 0.f, v3Floor.z + outv3Delta.z);

	float fHeightCur, fHeightNext;
	Vector3 v3NormalCur, v3NormalNext;
	const bool bInsideCur = pTerrain->GetHeightNormalWorld(v3SampleCur, fHeightCur, v3NormalCur);
	const bool bInsideNext = pTerrain->GetHeightNormalWorld(v3SampleNext, fHeightNext, v3NormalNext);

	if (!bInsideCur || !bInsideNext) {
		outHitResult.bGrounded = false;
		return;
	}

	float fHeight = fHeightNext;
	Vector3 v3Normal = v3NormalNext;

	if (v3Normal.y < 0.f) {
		v3Normal = -v3Normal;
	}
	v3Normal.Normalize();

	/*const float fGroundDot = bWasGrounded ? fLeaveGroundDot : fEnterGroundDot;
	if (v3Normal.y < fGroundDot) {
		outHitResult.bGrounded = false;
		outHitResult.v3Normal = v3Normal;
		outHitResult.fHeight = fHeight;
		outHitResult.fPenetrationDepth = 0.f;
		return;
	}*/

	float fDrop = fHeightCur - fHeightNext;
	fDrop = fDrop < 0.f ? 0.f : fDrop;

	float fSnapOnGround = fDrop + 1.0f;
	if (fSnapOnGround < fSnapOnAir) {
		fSnapOnGround = fSnapOnAir;
	}
	if (fSnapOnGround < fMaxSnapCap) {
		fSnapOnGround = fMaxSnapCap;
	}

	const float fAllowDown = bWasGrounded ? fSnapOnGround : fSnapOnAir;

	const float fTargetY = fHeight + fSkin;
	const float fPredictedY = v3Floor.y + outv3Delta.y;
	const float fDeltaToTarget = fTargetY - fPredictedY;

	if (fDeltaToTarget > 0.f) {
		outv3Delta.y += fDeltaToTarget;
		outHitResult.bGrounded = true;
	}
	else {
		const float fDown = -fDeltaToTarget;
		if (fDown <= fAllowDown) {
			outv3Delta.y += fDeltaToTarget;
			outHitResult.bGrounded = true;
		}
	}

	if (outHitResult.bGrounded) {
		outv3Delta.y -= 0.02f;

		outHitResult.v3Normal = v3Normal;
		outHitResult.fHeight = fHeight;
		outHitResult.fPenetrationDepth = (fDeltaToTarget > 0.f) ? fDeltaToTarget : 0.f;
	}
}
