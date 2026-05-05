#include "pch.h"
#include "Collider.h"

// ─── Triangle-Capsule 교차 헬퍼 ────────────────────────────────────────────

// 점 p에서 삼각형(a,b,c) 위의 가장 가까운 점 반환 (Ericson RTCD p.141)
static Vector3 ClosestPointOnTriangle(const Vector3& p, const Vector3& a, const Vector3& b, const Vector3& c)
{
	Vector3 ab = b - a, ac = c - a, ap = p - a;
	float d1 = ab.Dot(ap), d2 = ac.Dot(ap);
	if (d1 <= 0.f && d2 <= 0.f) return a;

	Vector3 bp = p - b;
	float d3 = ab.Dot(bp), d4 = ac.Dot(bp);
	if (d3 >= 0.f && d4 <= d3) return b;

	float vc = d1 * d4 - d3 * d2;
	if (vc <= 0.f && d1 >= 0.f && d3 <= 0.f)
		return a + (d1 / (d1 - d3)) * ab;

	Vector3 cp = p - c;
	float d5 = ab.Dot(cp), d6 = ac.Dot(cp);
	if (d6 >= 0.f && d5 <= d6) return c;

	float vb = d5 * d2 - d1 * d6;
	if (vb <= 0.f && d2 >= 0.f && d6 <= 0.f)
		return a + (d2 / (d2 - d6)) * ac;

	float va = d3 * d6 - d5 * d4;
	if (va <= 0.f && (d4 - d3) >= 0.f && (d5 - d6) >= 0.f)
		return b + ((d4 - d3) / ((d4 - d3) + (d5 - d6))) * (c - b);

	float denom = 1.f / (va + vb + vc);
	return a + ab * (vb * denom) + ac * (vc * denom);
}

// 점 p에서 선분(a,b) 위의 가장 가까운 점 반환
static Vector3 ClosestPointOnSegment(const Vector3& p, const Vector3& a, const Vector3& b)
{
	Vector3 ab = b - a;
	float denom = ab.Dot(ab);
	if (denom < 1e-8f) return a;
	float t = std::clamp((p - a).Dot(ab) / denom, 0.f, 1.f);
	return a + t * ab;
}

// 선분(p1,q1)과 선분(p2,q2) 사이의 최단 거리 제곱 (Ericson RTCD p.148)
static float ClosestDistSqSegSeg(
	const Vector3& p1, const Vector3& q1,
	const Vector3& p2, const Vector3& q2)
{
	Vector3 d1 = q1 - p1, d2 = q2 - p2, r = p1 - p2;
	float a = d1.Dot(d1), e = d2.Dot(d2), f = d2.Dot(r);
	float s, t;

	if (a < 1e-8f && e < 1e-8f)
		return r.Dot(r);

	if (a < 1e-8f) {
		s = 0.f;
		t = std::clamp(f / e, 0.f, 1.f);
	}
	else {
		float c = d1.Dot(r);
		if (e < 1e-8f) {
			t = 0.f;
			s = std::clamp(-c / a, 0.f, 1.f);
		}
		else {
			float b = d1.Dot(d2);
			float denom = a * e - b * b;
			s = (denom > 1e-8f) ? std::clamp((b * f - c * e) / denom, 0.f, 1.f) : 0.f;
			t = (b * s + f) / e;
			if (t < 0.f) { t = 0.f; s = std::clamp(-c / a, 0.f, 1.f); }
			else if (t > 1.f) { t = 1.f; s = std::clamp((b - c) / a, 0.f, 1.f); }
		}
	}

	Vector3 diff = (p1 + s * d1) - (p2 + t * d2);
	return diff.Dot(diff);
}

// 캡슐(선분 + 반지름)과 삼각형 교차 판정
static bool CapsuleIntersectsTriangle(
	const Vector3& segA, const Vector3& segB, float fRadius,
	const Vector3& triA, const Vector3& triB, const Vector3& triC)
{
	const float fRadSq = fRadius * fRadius;

	// 1. 캡슐 양 끝점 vs 삼각형
	Vector3 cA = ClosestPointOnTriangle(segA, triA, triB, triC);
	if (Vector3::DistanceSquared(segA, cA) <= fRadSq) return true;

	Vector3 cB = ClosestPointOnTriangle(segB, triA, triB, triC);
	if (Vector3::DistanceSquared(segB, cB) <= fRadSq) return true;

	// 2. 삼각형 꼭짓점 vs 캡슐 선분
	Vector3 cVA = ClosestPointOnSegment(triA, segA, segB);
	if (Vector3::DistanceSquared(triA, cVA) <= fRadSq) return true;

	Vector3 cVB = ClosestPointOnSegment(triB, segA, segB);
	if (Vector3::DistanceSquared(triB, cVB) <= fRadSq) return true;

	Vector3 cVC = ClosestPointOnSegment(triC, segA, segB);
	if (Vector3::DistanceSquared(triC, cVC) <= fRadSq) return true;

	// 3. 삼각형 각 변 vs 캡슐 선분
	if (ClosestDistSqSegSeg(segA, segB, triA, triB) <= fRadSq) return true;
	if (ClosestDistSqSegSeg(segA, segB, triB, triC) <= fRadSq) return true;
	if (ClosestDistSqSegSeg(segA, segB, triC, triA) <= fRadSq) return true;

	return false;
}

ICollider::ICollider(std::shared_ptr<IGameObject> pOwner)
	: IComponent{ pOwner }
{
}

void ICollider::MergeOBB(std::shared_ptr<IGameObject> pObj, bool bFixInWorld)
{
	if (m_bInitialized) {
		return;
	}

	auto pMeshRenderer = pObj->GetComponent<MeshRenderer>();
	if (pMeshRenderer) {
		if (m_xmOBBOrigin.Center == Vector3(0, 0, 0) && m_xmOBBOrigin.Extents == Vector3(1, 1, 1)) {
			pMeshRenderer->GetOBBMerged().Transform(m_xmOBBOrigin, pObj->GetWorldMatrix());
		}
		else {
			// Get corner fron OBB to merge
			XMFLOAT3 pxmf3OBBPoints1[BoundingOrientedBox::CORNER_COUNT];
			BoundingOrientedBox xmOBBMesh;
			if (bFixInWorld) {
				xmOBBMesh = pMeshRenderer->GetOBBMerged();
			}
			else {
				pMeshRenderer->GetOBBMerged().Transform(xmOBBMesh, pObj->GetWorldMatrix());
			}
			xmOBBMesh.GetCorners(pxmf3OBBPoints1);

			XMFLOAT3 pxmf3OBBPoints2[BoundingOrientedBox::CORNER_COUNT];
			m_xmOBBOrigin.GetCorners(pxmf3OBBPoints2);

			// Create AABB from OBB points for merge
			BoundingBox xmAABB1, xmAABB2;
			BoundingBox::CreateFromPoints(xmAABB1, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints1, sizeof(XMFLOAT3));
			BoundingBox::CreateFromPoints(xmAABB2, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBPoints2, sizeof(XMFLOAT3));

			// Merge OBB
			BoundingBox xmAABBMerged;
			BoundingBox::CreateMerged(xmAABBMerged, xmAABB1, xmAABB2);

			// Set OBB
			BoundingOrientedBox::CreateFromBoundingBox(m_xmOBBOrigin, xmAABBMerged);
		}
	}

	for (const auto& pChild : pObj->GetChildren()) {
		MergeOBB(pChild, bFixInWorld);
	}
}

bool ICollider::IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const
{
	return xmFrustumInWorld.Intersects(m_xmOBBWorld);
}

bool ICollider::IsInAABB(const BoundingBox& xmAABB) const
{
	return xmAABB.Intersects(m_xmOBBWorld);
}

bool ICollider::IsInOBB(const BoundingOrientedBox& xmOBB) const
{
	return xmOBB.Intersects(m_xmOBBWorld);
}

bool ICollider::CheckCollision(std::shared_ptr<ICollider> pOther) const
{
	return m_xmOBBWorld.Intersects(pOther->m_xmOBBWorld);
}

const BoundingBox ICollider::GetAABBFromOBBWorld() const
{
	BoundingBox xmRet;
	XMFLOAT3 pxmf3OBBCorners[BoundingOrientedBox::CORNER_COUNT];
	m_xmOBBWorld.GetCorners(pxmf3OBBCorners);
	BoundingBox::CreateFromPoints(xmRet, BoundingOrientedBox::CORNER_COUNT, pxmf3OBBCorners, sizeof(XMFLOAT3));
	return xmRet;
}

//////////////////////////////////////////////////////////////////////////////////////
// StaticCollider

StaticCollider::StaticCollider(std::shared_ptr<IGameObject> pOwner)
	:ICollider{ pOwner }
{
}

void StaticCollider::Initialize()
{
	auto& pOwner = m_wpOwner.lock();
	MergeOBB(pOwner, true);

	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	//m_xmOBBOrigin.Transform(m_xmOBBWorld, mtxWorld);
	m_xmOBBWorld = m_xmOBBOrigin;

	m_bInitialized = true;
}

void StaticCollider::Update()
{
	// Do nothing
}

std::shared_ptr<IComponent> StaticCollider::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	std::shared_ptr<StaticCollider> pClone = std::make_shared<StaticCollider>(pNewOwner);
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

//////////////////////////////////////////////////////////////////////////////////////
// DynamicCollider

DynamicCollider::DynamicCollider(std::shared_ptr<IGameObject> pOwner)
	:ICollider{ pOwner }
{
}

void DynamicCollider::Initialize()
{
	auto& pOwner = m_wpOwner.lock();
	MergeOBB(pOwner, false);

	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_xmOBBOrigin.Transform(m_xmOBBWorld, mtxWorld);

	m_bInitialized = true;
}

void DynamicCollider::Update()
{
	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_xmOBBOrigin.Transform(m_xmOBBWorld, mtxWorld);
}

std::shared_ptr<IComponent> DynamicCollider::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	std::shared_ptr<DynamicCollider> pClone = std::make_shared<DynamicCollider>(pNewOwner);
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

//////////////////////////////////////////////////////////////////////////////////////
// PlayerCollider

PlayerCollider::PlayerCollider(std::shared_ptr<IGameObject> pOwner)
	:ICollider{ pOwner }
{
}

void PlayerCollider::Initialize()
{
	auto& pOwner = m_wpOwner.lock();
	MergeOBB(pOwner, false);

	BoundingCapsule::CreateFromBoundingOrientedBox(m_CapsuleOrigin, m_xmOBBOrigin);

	// 반지름 재조정 필요
	//m_CapsuleOrigin.fRadius = std::min(m_xmOBBOrigin.Extents.x, m_xmOBBOrigin.Extents.z) * 2;

	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_CapsuleOrigin.Transform(m_CapsuleWorld, mtxWorld);

	m_bInitialized = true;
}

void PlayerCollider::Update()
{
	const Matrix& mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	m_CapsuleOrigin.Transform(m_CapsuleWorld, mtxWorld);
}

std::shared_ptr<IComponent> PlayerCollider::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	std::shared_ptr<PlayerCollider> pClone = std::make_shared<PlayerCollider>(pNewOwner);
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_CapsuleOrigin = m_CapsuleOrigin;
	pClone->m_CapsuleWorld = m_CapsuleWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

bool PlayerCollider::IsInFrustum(const BoundingFrustum& xmFrustumInWorld) const
{
	BoundingBox xmAABB;
	m_CapsuleWorld.CreateAABBFromCapsule(xmAABB);

	return xmFrustumInWorld.Intersects(xmAABB);
}

bool PlayerCollider::IsInAABB(const BoundingBox& xmAABBWorld) const
{
	BoundingBox xmAABB;
	m_CapsuleWorld.CreateAABBFromCapsule(xmAABB);

	return xmAABBWorld.Intersects(xmAABB);
}

bool PlayerCollider::IsInOBB(const BoundingOrientedBox& xmOBB) const
{
	BoundingBox xmAABB;
	m_CapsuleWorld.CreateAABBFromCapsule(xmAABB);

	return xmOBB.Intersects(xmAABB);
}

bool PlayerCollider::CheckCollision(std::shared_ptr<ICollider> pOther) const
{
	return m_CapsuleWorld.Intersects(pOther->GetOBBWorld());
}

//////////////////////////////////////////////////////////////////////////////////////
// MeshCollider

MeshCollider::MeshCollider(std::shared_ptr<IGameObject> pOwner, COLLISIONMESHINFO info)
	: ICollider{ pOwner }
	, m_unIndices{ std::move(info.unIndices) }
	, m_strType{ std::move(info.strType) }
{
	m_v3BakedVertices = std::move(info.v3Positions);
}

void MeshCollider::Initialize()
{
	if (m_bInitialized)
		return;

	// 정점을 월드 공간으로 한 번만 변환해서 굽기
	const Matrix mtxWorld = m_wpOwner.lock()->GetWorldMatrix();
	for (auto& v : m_v3BakedVertices)
		v = Vector3::Transform(v, mtxWorld);

	// 구워진 월드 정점에서 OBB 생성
	// CreateFromPoints는 평면/공선 점집합에서 비단위 사원수를 생성할 수 있으므로
	// AABB로 먼저 만든 뒤 OBB로 변환 (identity 사원수 보장)
	BoundingBox xmAABB;
	BoundingBox::CreateFromPoints(
		xmAABB,
		m_v3BakedVertices.size(),
		m_v3BakedVertices.data(),
		sizeof(Vector3)
	);
	BoundingOrientedBox::CreateFromBoundingBox(m_xmOBBOrigin, xmAABB);
	m_xmOBBWorld = m_xmOBBOrigin;

	m_bInitialized = true;
}

void MeshCollider::Update()
{
	// Static이므로 아무것도 하지 않음
}

std::shared_ptr<IComponent> MeshCollider::Copy(std::shared_ptr<IGameObject> pNewOwner) const
{
	COLLISIONMESHINFO info;
	info.strType = m_strType;
	info.v3Positions = m_v3BakedVertices;
	info.unIndices = m_unIndices;

	auto pClone = std::make_shared<MeshCollider>(pNewOwner, std::move(info));
	pClone->m_xmOBBOrigin = m_xmOBBOrigin;
	pClone->m_xmOBBWorld = m_xmOBBWorld;
	pClone->m_bInitialized = m_bInitialized;

	return pClone;
}

bool MeshCollider::CheckCollision(std::shared_ptr<ICollider> pOther) const
{
	auto pPlayerCollider = std::dynamic_pointer_cast<PlayerCollider>(pOther);
	if (pPlayerCollider) {
		// PlayerCollider는 m_xmOBBWorld를 Update()에서 갱신하지 않으므로
		// 캡슐 AABB로 broad-phase 수행
		BoundingBox xmCapsuleAABB;
		pPlayerCollider->GetCapsuleWorld().CreateAABBFromCapsule(xmCapsuleAABB);
		if (!m_xmOBBWorld.Intersects(xmCapsuleAABB))
			return false;

		m_LastContacts.clear();
		return CheckCapsuleVsTriangles(pPlayerCollider->GetCapsuleWorld(), m_LastContacts);
	}

	// 그 외 OBB 기반 콜라이더: OBB broad-phase만
	return m_xmOBBWorld.Intersects(pOther->GetOBBWorld());
}

bool MeshCollider::CheckCapsuleVsTriangles(const BoundingCapsule& capsule,
	std::vector<std::pair<Vector3, float>>& outContacts) const
{
	const Vector3 v3SegTop = capsule.v3Center + Vector3(0.f, capsule.fHalfHeight, 0.f);
	const Vector3 v3SegBot = capsule.v3Center - Vector3(0.f, capsule.fHalfHeight, 0.f);
	const float fRadius = capsule.fRadius;
	const float fFloorDot = 0.7f;	// ResolveCollision의 fGround와 동일 기준

	// 바닥(normal.y > fFloorDot)과 벽/경사 카테고리 각각 최대 depth contact 추적
	bool bFloor = false, bWall = false;
	float fBestFloorDepth = 0.f;
	Vector3 v3BestFloorNormal = Vector3{ 0.f, 1.f, 0.f };
	float fBestWallDepth = 0.f;
	Vector3 v3BestWallNormal = Vector3{ 1.f, 0.f, 0.f };

	const uint32 nTriCount = static_cast<uint32>(m_unIndices.size()) / 3;
	for (uint32 i = 0; i < nTriCount; ++i) {
		const Vector3& a = m_v3BakedVertices[m_unIndices[i * 3 + 0]];
		const Vector3& b = m_v3BakedVertices[m_unIndices[i * 3 + 1]];
		const Vector3& c = m_v3BakedVertices[m_unIndices[i * 3 + 2]];

		if (!CapsuleIntersectsTriangle(v3SegTop, v3SegBot, fRadius, a, b, c))
			continue;

		// 삼각형 face normal 계산
		Vector3 v3FaceNormal = (b - a).Cross(c - a);
		float fLen = v3FaceNormal.Length();
		if (fLen < 1e-6f) continue;
		v3FaceNormal /= fLen;

		// 캡슐 방향으로 법선 정렬
		if ((capsule.v3Center - a).Dot(v3FaceNormal) < 0.f)
			v3FaceNormal = -v3FaceNormal;

		// 삼각형 평면과 캡슐 세그먼트 양 끝점의 부호 있는 거리 중 최솟값
		// (max(0, ...) 제거: spine이 지오메트리 내부로 들어간 경우에도 depth를 정확히 계산)
		float fDistTop = (v3SegTop - a).Dot(v3FaceNormal);
		float fDistBot = (v3SegBot - a).Dot(v3FaceNormal);
		float fMinDist = std::min(fDistTop, fDistBot);
		float fDepth = fRadius - fMinDist;

		if (fDepth <= 0.f) continue;	// 실제 관통 없음 (엣지 스침 등), 스킵

		// 바닥/벽 분류하여 각 카테고리의 최대 depth contact 갱신
		if (v3FaceNormal.y > fFloorDot) {
			if (fDepth > fBestFloorDepth) {
				fBestFloorDepth = fDepth;
				v3BestFloorNormal = v3FaceNormal;
				bFloor = true;
			}
		}
		else {
			if (fDepth > fBestWallDepth) {
				fBestWallDepth = fDepth;
				v3BestWallNormal = v3FaceNormal;
				bWall = true;
			}
		}
	}

	if (bFloor)
		outContacts.emplace_back(v3BestFloorNormal, fBestFloorDepth);
	if (bWall)
		outContacts.emplace_back(v3BestWallNormal, fBestWallDepth);

	return bFloor || bWall;
}

bool MeshCollider::TestCapsule(const BoundingCapsule& capsule) const
{
	BoundingBox xmCapsuleAABB;
	capsule.CreateAABBFromCapsule(xmCapsuleAABB);
	if (!m_xmOBBWorld.Intersects(xmCapsuleAABB))
		return false;

	std::vector<std::pair<Vector3, float>> contacts;
	return CheckCapsuleVsTriangles(capsule, contacts);
}
