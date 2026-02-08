#pragma once

struct BoundingCapsule {
	Vector3 v3Center;
	float fHalfHeight;
	float fRadius;

	bool Intersects(const BoundingOrientedBox& box) const noexcept;
	bool Intersects(const BoundingBox& box) const noexcept;
	bool Intersects(const BoundingOrientedBox& box, OUT Vector3& outv3Normal, OUT float& outfDepth) const;
	bool Intersects(const BoundingBox& box, OUT Vector3& outv3Normal, OUT float& outfDepth) const;

	void Transform(OUT BoundingCapsule& out, const DirectX::XMMATRIX mtxTransform) const noexcept;

	void CreateAABBFromCapsule(OUT BoundingBox& out) const;

	static void CreateFromBoundingBox(OUT BoundingCapsule& outCapsule, const BoundingBox& xmAABB);
	static void CreateFromBoundingOrientedBox(OUT BoundingCapsule& outCapsule, const BoundingOrientedBox& xmOBB);

private:
	void GetSegment(OUT Vector3& outv3Down, OUT Vector3& outv3Up) const;
	Vector3 ClosestPointsOnOBB(const Vector3& v3Point, const BoundingOrientedBox& box) const;
	bool SegmentIntersectOBB(
		const Vector3& v3Seg0,
		const Vector3& v3Seg1,
		float fRadius,
		const BoundingOrientedBox& xmOBB) const;

	bool SegmentIntersectOBBWithPenetrationDepth(
		const Vector3& v3Seg0,
		const Vector3& v3Seg1,
		float fRadius,
		const BoundingOrientedBox& xmOBB,
		OUT Vector3& outv3Normal,
		OUT float& outfDepth) const;

};

