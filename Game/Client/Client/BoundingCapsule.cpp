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
		fHalfHeight,
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
	const uint32 unStepCount = 4;

	float fBestDepthSq = std::numeric_limits<float>::max();
	Vector3 v3BestNormal = Vector3::Zero;

	for (uint32 i = 0; i < unStepCount; ++i) {
		float fStep = (float)i / (unStepCount - 1);

		Vector3 v3Point = Vector3::Lerp(v3Seg0, v3Seg1, fStep);
		Vector3 v3Closest = ClosestPointsOnOBB(v3Point, xmOBB);
		Vector3 v3Direction = v3Point - v3Closest;

		float fDepthSq = v3Direction.LengthSquared();

		if (fDepthSq < fBestDepthSq) {
			fBestDepthSq = fDepthSq;
			v3BestNormal = v3Direction;
		}
	}

	float fRadiusSq = fRadius * fRadius;
	if (fBestDepthSq > fRadiusSq) {
		return false;
	}

	float fDepth = std::sqrtf(fBestDepthSq);
	outfDepth = fRadius - fDepth;

	if (fDepth > 1e-5f) {
		outv3Normal = v3BestNormal / fDepth;
	}
	else {
		outv3Normal = Vector3(0, 1, 0);
	}

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

