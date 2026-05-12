#pragma once

// 클라이언트 BoundingCapsule 과 동일 구조 (서버용 경량 버전 — Ray 교차만 사용)

struct ServerBoundingCapsule {
	Vector3 v3Center    = {};
	float   fHalfHeight = 0.f;
	float   fRadius     = 0.f;

	bool Intersects(const Vector3& v3RayOrigin, const Vector3& v3RayDir, float& outDist) const;

	void GetSegment(Vector3& outDown, Vector3& outUp) const;
};
