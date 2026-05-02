#include "pch.h"
#include "BoundingCapsule.h"

bool BoundingCapsule::Intersects(const BoundingOrientedBox& box) const noexcept
{
	Vector3 v3Seg0, v3Seg1;
	GetSegment(v3Seg0, v3Seg1);

	return SegmentIntersectOBB(v3Seg0, v3Seg1, fRadius, box);
}

bool BoundingCapsule::Intersects(const BoundingBox& box) const noexcept
{
	Vector3 v3Seg0, v3Seg1;
	GetSegment(v3Seg0, v3Seg1);

	BoundingOrientedBox xmOBB;
	BoundingOrientedBox::CreateFromBoundingBox(xmOBB, box);

	return SegmentIntersectOBB(v3Seg0, v3Seg1, fRadius, xmOBB);
}

bool BoundingCapsule::Intersects(const BoundingOrientedBox& box, OUT Vector3& outv3Normal, OUT float& outfDepth) const
{
	Vector3 v3Seg0, v3Seg1;
	GetSegment(v3Seg0, v3Seg1);

	return SegmentIntersectOBBWithPenetrationDepth(v3Seg0, v3Seg1, fRadius, box, outv3Normal, outfDepth);
}

bool BoundingCapsule::Intersects(const BoundingBox& box, OUT Vector3& outv3Normal, OUT float& outfDepth) const
{
	Vector3 v3Seg0, v3Seg1;
	GetSegment(v3Seg0, v3Seg1);

	BoundingOrientedBox xmOBB;
	BoundingOrientedBox::CreateFromBoundingBox(xmOBB, box);

	return SegmentIntersectOBBWithPenetrationDepth(v3Seg0, v3Seg1, fRadius, xmOBB, outv3Normal, outfDepth);
}

void BoundingCapsule::Transform(OUT BoundingCapsule& out, const DirectX::XMMATRIX mtxTransform) const noexcept
{
	out.v3Center = Vector3::Transform(v3Center, mtxTransform);
	
	// Scale
	float fScale = XMVectorGetX(XMVector3Length(mtxTransform.r[0]));
	out.fRadius = fRadius * fScale;
	out.fHalfHeight= fHalfHeight * fScale;
}

void BoundingCapsule::CreateAABBFromCapsule(OUT BoundingBox& out) const
{
	out.Center = v3Center;
	out.Extents = Vector3{
		fRadius,
		fHalfHeight + fRadius,
		fRadius
	};
}

void BoundingCapsule::GetSegment(OUT Vector3& outv3Down, OUT Vector3& outv3Up) const
{
	Vector3 v3Up = Vector3::Up;

	outv3Down = v3Center - v3Up * fHalfHeight;
	outv3Up = v3Center + v3Up * fHalfHeight;
}

Vector3 BoundingCapsule::ClosestPointsOnOBB(const Vector3& v3Point, const BoundingOrientedBox& box) const
{
	Vector3 v3DirectionToBox = v3Point - box.Center;

	Vector3 pv3Axis[3] = {
		 XMVector3Rotate(g_XMIdentityR0, XMQuaternionNormalize(XMLoadFloat4(&box.Orientation))),
		 XMVector3Rotate(g_XMIdentityR1, XMQuaternionNormalize(XMLoadFloat4(&box.Orientation))),
		 XMVector3Rotate(g_XMIdentityR2, XMQuaternionNormalize(XMLoadFloat4(&box.Orientation)))
	};

	Vector3 v3Result = box.Center;
	const float pfBoxExtent[3] = {
		box.Extents.x,
		box.Extents.y,
		box.Extents.z,
	};

	for (uint32 i = 0; i < 3; ++i) {
		float fDistance = v3DirectionToBox.Dot(pv3Axis[i]);
		fDistance = std::clamp(fDistance, -pfBoxExtent[i], pfBoxExtent[i]);
		v3Result += pv3Axis[i] * fDistance;
	}

	return v3Result;
}

bool BoundingCapsule::SegmentIntersectOBB(const Vector3& v3Seg0, const Vector3& v3Seg1, float fRadius, const BoundingOrientedBox& xmOBB) const
{
	const uint32 unStepCount = 4;
	float fMinDepthSq = std::numeric_limits<float>::max();

	for (uint32 i = 0; i < unStepCount; ++i) {
		float fStep = (float)i / (unStepCount - 1);

		Vector3 v3Point = Vector3::Lerp(v3Seg0, v3Seg1, fStep);
		Vector3 v3Closest = ClosestPointsOnOBB(v3Point, xmOBB);
		Vector3 v3Direction = v3Point - v3Closest;

		float fDepthSq = v3Direction.LengthSquared();

		fMinDepthSq = std::min(fMinDepthSq, fDepthSq);
	}

	return fMinDepthSq <= fRadius * fRadius;
}

bool BoundingCapsule::SegmentIntersectOBBWithPenetrationDepth(const Vector3& v3Seg0, const Vector3& v3Seg1, float fRadius, const BoundingOrientedBox& xmOBB, OUT Vector3& outv3Normal, OUT float& outfDepth) const
{
	// OBB 로컬 공간(= AABB [-extents, +extents])으로 세그먼트 변환
	// → 이산화 없이 정확한 세그먼트-OBB 최근접점을 계산
	XMVECTOR qOri  = XMQuaternionNormalize(XMLoadFloat4(&xmOBB.Orientation));
	XMVECTOR qConj = XMQuaternionConjugate(qOri);

	const Vector3 v3OBBCenter = xmOBB.Center;
	const Vector3 v3E         = xmOBB.Extents;   // half-extents

	const Vector3 localA = XMVector3Rotate(v3Seg0 - v3OBBCenter, qConj);
	const Vector3 localB = XMVector3Rotate(v3Seg1 - v3OBBCenter, qConj);
	const Vector3 d      = localB - localA;

	// 캡슐 세그먼트 위의 점 P(t)=localA+t*d 에서 AABB [-e,+e] 까지의 거리 제곱을
	// 최소화하는 t를 찾는다.
	// f(t) 는 각 축별 max(0, |P(t)[i]| - e[i])^2 의 합이므로
	// 극값은 반드시 t=0, t=1, 또는 d[i]≠0 인 경우 6개 면 교점 t=±e[i]-a[i]/d[i] 에서만 발생한다.
	auto evalDistSq = [&](float t) -> float {
		t = std::clamp(t, 0.f, 1.f);
		const Vector3 p = localA + d * t;
		const Vector3 clamped{
			std::clamp(p.x, -v3E.x, v3E.x),
			std::clamp(p.y, -v3E.y, v3E.y),
			std::clamp(p.z, -v3E.z, v3E.z)
		};
		return Vector3::DistanceSquared(p, clamped);
	};

	// 후보 t 목록: 양 끝점 + 각 축별 ±face 교점
	float afCandidates[8];
	uint32 unCount = 0;
	afCandidates[unCount++] = 0.f;
	afCandidates[unCount++] = 1.f;

	const float* pA = &localA.x;
	const float* pD = &d.x;
	const float* pE = &v3E.x;
	for (int i = 0; i < 3; ++i) {
		if (std::abs(pD[i]) > 1e-8f) {
			float t1 = (-pE[i] - pA[i]) / pD[i];
			float t2 = (+pE[i] - pA[i]) / pD[i];
			if (t1 >= 0.f && t1 <= 1.f) afCandidates[unCount++] = t1;
			if (t2 >= 0.f && t2 <= 1.f) afCandidates[unCount++] = t2;
		}
	}

	float fBestDistSq = std::numeric_limits<float>::max();
	float fBestT      = 0.f;
	for (uint32 i = 0; i < unCount; ++i) {
		float fDistSq = evalDistSq(afCandidates[i]);
		if (fDistSq < fBestDistSq) {
			fBestDistSq = fDistSq;
			fBestT      = std::clamp(afCandidates[i], 0.f, 1.f);
		}
	}

	const float fRadiusSq = fRadius * fRadius;
	if (fBestDistSq > fRadiusSq) return false;

	// 최근접 점 쌍을 월드 공간으로 복원해 법선 계산
	const Vector3 localP = localA + d * fBestT;
	const Vector3 localClamped{
		std::clamp(localP.x, -v3E.x, v3E.x),
		std::clamp(localP.y, -v3E.y, v3E.y),
		std::clamp(localP.z, -v3E.z, v3E.z)
	};

	const Vector3 v3WorldP       = v3OBBCenter + Vector3(XMVector3Rotate(localP,       qOri));
	const Vector3 v3WorldClamped = v3OBBCenter + Vector3(XMVector3Rotate(localClamped, qOri));
	const Vector3 v3Dir          = v3WorldP - v3WorldClamped;
	const float   fDist          = std::sqrtf(fBestDistSq);

	outfDepth = fRadius - fDist;
	outv3Normal = (fDist > 1e-5f) ? v3Dir / fDist : Vector3(0.f, 1.f, 0.f);

	return true;
}

void BoundingCapsule::CreateFromBoundingBox(OUT BoundingCapsule& outCapsule, const BoundingBox& xmAABB)
{
	outCapsule.v3Center = xmAABB.Center;

	float fRadius = std::min(xmAABB.Extents.x, xmAABB.Extents.z);

	outCapsule.fRadius = fRadius;
	outCapsule.fHalfHeight = std::max(0.f, xmAABB.Extents.y - fRadius);
}

void BoundingCapsule::CreateFromBoundingOrientedBox(OUT BoundingCapsule& outCapsule, const BoundingOrientedBox& xmOBB)
{
	BoundingBox xmAABB;
	CreateAABBFromOBB(xmAABB, xmOBB);

	CreateFromBoundingBox(outCapsule, xmAABB);
}

