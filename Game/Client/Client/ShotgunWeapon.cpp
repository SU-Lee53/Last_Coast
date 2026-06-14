#include "pch.h"
#include "ShotgunWeapon.h"

#include "StaticObject.h"
#include "Zombie.h"

void ShotgunWeapon::FireShot(const Vector3& v3CamPos, const Vector3& v3CamDir, bool bOnline)
{
	const auto& pCamera = CUR_SCENE->GetCamera();
	const Vector3 v3Right = pCamera->GetRight();
	const Vector3 v3Up    = pCamera->GetUp();

	const float fSpread = XMConvertToRadians(m_fSpreadDegree);

	for (int32 n = 0; n < m_nPelletCount; ++n) {
		
		float fx = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.f * fSpread;
		float fy = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.f * fSpread;

		Vector3 v3Dir = v3CamDir + v3Right * fx + v3Up * fy;
		v3Dir.Normalize();

		if (bOnline) {
			
			NETWORK->SendPlayerShoot(v3CamPos, v3Dir, m_v3MuzzlePositionWorld, m_fDamage);
		}
		else {

			RayTraceDesc rayDesc{};
			rayDesc.v3Origin = v3CamPos;
			rayDesc.v3Direction = v3Dir;
			rayDesc.fMaxDistance = 5000.f;
			rayDesc.fDamage = m_fDamage;
			rayDesc.pInstigator = m_wpOwner.lock().get();
			rayDesc.pSourceObject = this;

			RayTraceHitResult hit{};
			CUR_SCENE->GetWorld().LineTraceSingle<StaticObject, Zombie>(rayDesc, hit);
		}
	}
}
