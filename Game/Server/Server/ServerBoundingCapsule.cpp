#include "pch.h"
#include "ServerBoundingCapsule.h"

void ServerBoundingCapsule::GetSegment(Vector3& outDown, Vector3& outUp) const
{
	outDown = v3Center - Vector3::Up * fHalfHeight;
	outUp   = v3Center + Vector3::Up * fHalfHeight;
}

bool ServerBoundingCapsule::Intersects(const Vector3& v3RayOrigin, const Vector3& v3RayDir, float& outDist) const
{
	outDist = 0.f;

	Vector3 dir = v3RayDir;
	float dirLenSq = dir.LengthSquared();
	if (dirLenSq <= 1e-8f)
		return false;

	dir.Normalize();

	constexpr float EPS = 1e-6f;

	float fRad = fRadius;
	float fRadSq = fRad * fRad;

	Vector3 v3Bottom = v3Center - Vector3::Up * fHalfHeight;
	Vector3 v3Top = v3Center + Vector3::Up * fHalfHeight;

	// 1. 레이 시작점이 캡슐 내부에 있으면 거리 0
	{
		float y = std::clamp(v3RayOrigin.y, v3Bottom.y, v3Top.y);
		Vector3 v3Closest{ v3Center.x, y, v3Center.z };
		Vector3 v3Diff = v3RayOrigin - v3Closest;
		if (v3Diff.LengthSquared() <= fRadSq) {
			outDist = 0.f;
			return true;
		}
	}

	float fBestT = std::numeric_limits<float>::max();
	bool bHit = false;

	// 2. 무한 실린더 측면 교차 → Y 범위 클램핑
	{
		float ox = v3RayOrigin.x - v3Center.x;
		float oz = v3RayOrigin.z - v3Center.z;
		float dx = dir.x;
		float dz = dir.z;

		float a = dx * dx + dz * dz;
		float b = 2.f * (ox * dx + oz * dz);
		float c = ox * ox + oz * oz - fRadSq;

		if (std::abs(a) > EPS) {
			float fDiscriminant = b * b - 4.f * a * c;

			if (fDiscriminant >= 0.f) {
				float fSqrtD = std::sqrt(fDiscriminant);
				float fInv2A = 1.f / (2.f * a);

				float t0 = (-b - fSqrtD) * fInv2A;
				float t1 = (-b + fSqrtD) * fInv2A;

				// t0 검사
				if (t0 >= 0.f) {
					float y = v3RayOrigin.y + dir.y * t0;
					if (y >= v3Bottom.y && y <= v3Top.y) {
						if (t0 < fBestT) { fBestT = t0; bHit = true; }
					}
				}
				// t1 검사
				if (t1 >= 0.f) {
					float y = v3RayOrigin.y + dir.y * t1;
					if (y >= v3Bottom.y && y <= v3Top.y) {
						if (t1 < fBestT) { fBestT = t1; bHit = true; }
					}
				}
			}
		}
	}

	// 3. 하단 반구
	{
		Vector3 v3M = v3RayOrigin - v3Bottom;
		float b = v3M.Dot(dir);
		float c = v3M.Dot(v3M) - fRadSq;
		float fDisc = b * b - c;

		if (fDisc >= 0.f) {
			float fSqrtD = std::sqrt(fDisc);
			float t0 = -b - fSqrtD;
			float t1 = -b + fSqrtD;

			if (t0 >= 0.f && t0 < fBestT) { fBestT = t0; bHit = true; }
			if (t1 >= 0.f && t1 < fBestT) { fBestT = t1; bHit = true; }
		}
	}

	// 4. 상단 반구
	{
		Vector3 v3M = v3RayOrigin - v3Top;
		float b = v3M.Dot(dir);
		float c = v3M.Dot(v3M) - fRadSq;
		float fDisc = b * b - c;

		if (fDisc >= 0.f) {
			float fSqrtD = std::sqrt(fDisc);
			float t0 = -b - fSqrtD;
			float t1 = -b + fSqrtD;

			if (t0 >= 0.f && t0 < fBestT) { fBestT = t0; bHit = true; }
			if (t1 >= 0.f && t1 < fBestT) { fBestT = t1; bHit = true; }
		}
	}

	if (!bHit)
		return false;

	outDist = fBestT;
	return true;
}
