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

bool BoundingCapsule::Intersects(const Vector3& v3RayOrigin, const Vector3& v3RayDir, OUT float& outDist) const
{
	outDist = 0.f;

	Vector3 dir = v3RayDir;
	const float dirLenSq = dir.LengthSquared();
	if (dirLenSq <= 1e-8f) {
		return false;
	}

	dir.Normalize();

	constexpr float EPS = 1e-6f;

	const float radius = fRadius;
	const float radiusSq = radius * radius;

	// Segment = center + [-halfHeight, +halfHeight] on Y axis.
	const Vector3 bottom = v3Center - Vector3::Up * fHalfHeight;
	const Vector3 top = v3Center + Vector3::Up * fHalfHeight;

	// 1. If ray starts inside capsule, hit distance is 0.
	{
		float y = std::clamp(v3RayOrigin.y, bottom.y, top.y);
		Vector3 closestOnSegment{ v3Center.x, y, v3Center.z };

		Vector3 diff = v3RayOrigin - closestOnSegment;
		if (diff.LengthSquared() <= radiusSq) {
			outDist = 0.f;
			return true;
		}
	}

	float bestT = std::numeric_limits<float>::max();
	bool hit = false;

	auto tryAddHit = [&](float t)
		{
			if (t >= 0.f && t < bestT) {
				bestT = t;
				hit = true;
			}
		};

	// 2. Infinite cylinder side intersection, then clamp by Y range.
	// Cylinder axis: Y
	// Equation: (x - cx)^2 + (z - cz)^2 = r^2
	{
		const float ox = v3RayOrigin.x - v3Center.x;
		const float oz = v3RayOrigin.z - v3Center.z;

		const float dx = dir.x;
		const float dz = dir.z;

		const float a = dx * dx + dz * dz;
		const float b = 2.f * (ox * dx + oz * dz);
		const float c = ox * ox + oz * oz - radiusSq;

		if (std::abs(a) > EPS) {
			const float discriminant = b * b - 4.f * a * c;

			if (discriminant >= 0.f) {
				const float sqrtD = std::sqrt(discriminant);
				const float inv2A = 1.f / (2.f * a);

				const float t0 = (-b - sqrtD) * inv2A;
				const float t1 = (-b + sqrtD) * inv2A;

				auto testCylinderT = [&](float t)
					{
						if (t < 0.f) {
							return;
						}

						const float y = v3RayOrigin.y + dir.y * t;
						if (y >= bottom.y && y <= top.y) {
							tryAddHit(t);
						}
					};

				testCylinderT(t0);
				testCylinderT(t1);
			}
		}
	}

	// 3. Sphere cap intersection helper.
	auto testSphere = [&](const Vector3& sphereCenter)
		{
			const Vector3 m = v3RayOrigin - sphereCenter;

			const float b = m.Dot(dir);
			const float c = m.Dot(m) - radiusSq;

			const float discriminant = b * b - c;
			if (discriminant < 0.f) {
				return;
			}

			const float sqrtD = std::sqrt(discriminant);

			const float t0 = -b - sqrtD;
			const float t1 = -b + sqrtD;

			tryAddHit(t0);
			tryAddHit(t1);
		};

	// 4. Bottom and top hemispheres.
	testSphere(bottom);
	testSphere(top);

	if (!hit) {
		return false;
	}

	outDist = bestT;
	return true;
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

