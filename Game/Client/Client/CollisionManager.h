#pragma once
#include "GameObject.h"

/*
	- Dynamic (Player) vs Static 간의 충돌이 주요 쟁점
	- 별도의 Collider 객체의 필요여부는 좀 더 고민해볼 것

	목표
	- GameObject 등록
	- 등록된 GameObject 간 충돌처리
	- 충돌(Overlap)된 객체의 보관 -> Begin - While - End 의 구조

	@@ Register 된 객체와 충돌 일어난 객체들의 보관과 구분에 대한 자료구조는 뭘 사용해야 하는가 @@

	추후 목표 (생각중)
	- 공간분할 (거의 필수)
	- Overlap 이 아닌 Hit 충돌처리 (아직 모름)
		- 충돌위치, 충격량 등

*/

struct CollisionResult {
	std::shared_ptr<GameObject> pSelf;
	std::shared_ptr<GameObject> pOther;
};

class CollisionManager {
	
	DECLARE_SINGLE(CollisionManager);

public:
	template<typename T> requires std::derived_from<T, GameObject>
	void RegisterObject(std::shared_ptr<GameObject> pObj);
	void CheckCollision();

private:
	std::vector<std::shared_ptr<GameObject>> m_pRegisteredStaticObjects;
	std::vector<std::shared_ptr<GameObject>> m_pRegisteredDynamicObjects;


};

template<typename T> requires std::derived_from<T, GameObject>
inline void CollisionManager::RegisterObject(std::shared_ptr<GameObject> pObj)
{

}
