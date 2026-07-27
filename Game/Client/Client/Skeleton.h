#pragma once
#include "Component.h"

class WeaponObject;
class Skeleton;

interface IAttachSocket abstract{
public:
	IAttachSocket(std::shared_ptr<Skeleton> pOwner, int32 nBoneIndex)
		: m_wpOwner{ pOwner }, m_nAttachedBoneIndex{ nBoneIndex } {};
	virtual ~IAttachSocket() {}

	virtual void Initialize () = 0;
	virtual void Update() = 0;
	virtual void Render() = 0;

	std::weak_ptr<Skeleton> m_wpOwner;
	int32 m_nAttachedBoneIndex;
	Matrix m_mtxTransform = Matrix::Identity;
};

class WeaponSocket : public IAttachSocket {
public:
	WeaponSocket(std::shared_ptr<Skeleton> pOwner, int32 nBoneIndex);
	virtual ~WeaponSocket() {}

	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Render() override;

	bool TryFire();

	void SetWeapon(WEAPON_TYPE eWeaponType);
	// 미리 만들어둔 무기 인스턴스 장착 — 슬롯 전환 시 탄약 유지용 (SetWeapon은 매번 새 복사본)
	void SetWeaponObject(const std::shared_ptr<WeaponObject>& pWeapon, WEAPON_TYPE eWeaponType);
	std::shared_ptr<WeaponObject> GetWeaponModel() const { return m_pWeaponModel; };

	WEAPON_TYPE GetCurrentWeaponType() const { return m_eCurrentWeapon; }

	// 무기 모델 표시 여부 — 붕대 들기처럼 빈손이어야 할 때 렌더만 끈다 (장착/탄약 상태는 유지)
	void SetModelVisible(bool bVisible) { m_bModelVisible = bVisible; }
	bool IsModelVisible() const { return m_bModelVisible; }

private:
	WEAPON_TYPE m_eCurrentWeapon;
	bool m_bModelVisible = true;

	Vector3 m_v3OffsetPosition = Vector3::Zero;
	Vector3 m_v3OffsetRotation = Vector3(180.f, -90.f, -90.f);

	std::shared_ptr<WeaponObject> m_pWeaponModel = nullptr;

};

// 오른손 수류탄 모델 소켓 — 수류탄 들기/와인드업 동안만 표시.
// WeaponSocket과 동일한 본 부착 변환, 모델은 granade.bin NodeObject.
class GrenadeHandSocket : public IAttachSocket {
public:
	GrenadeHandSocket(std::shared_ptr<Skeleton> pOwner, int32 nBoneIndex)
		: IAttachSocket{ pOwner, nBoneIndex } {}
	virtual ~GrenadeHandSocket() {}

	virtual void Initialize() override;
	virtual void Update() override;
	virtual void Render() override;

	void SetVisible(bool bVisible) { m_bVisible = bVisible; }
	bool IsVisible() const { return m_bVisible; }
	std::shared_ptr<IGameObject> GetModel() const { return m_pModel; }	// 그림자 캐스터 수집용

	// 손 모델 전환 — 일반(granade) ↔ 디코이(stun_grenade). 모델은 종류별 지연 생성 후 캐시
	void SetDecoy(bool bDecoy);

	void EditOffsetImGui();	// 손 안 위치/자세/크기 실시간 튜닝 (Skeleton ImGui 패널에서 호출)
	virtual const char* SocketLabel() const { return "Grenade Hand Socket"; }	// ImGui 표시명 (파생 소켓이 덮어씀)

protected:
	// 래퍼 루트에 소켓 행렬을 쓰고 모델은 자식으로 붙여, 모델 루트에 구워진 변환 보존
	static std::shared_ptr<IGameObject> CreateWrappedModel(const std::string& modelName);

	bool m_bVisible = false;
	bool m_bDecoy = false;

	// 손 안 위치/자세 조정용 (도, cm) — 인게임 튜닝으로 확정한 값
	Vector3 m_v3OffsetPosition = Vector3{ 5.f, 10.f, 0.f };
	Vector3 m_v3OffsetRotation = Vector3::Zero;
	float   m_fScale = 1.f;

	// 종류별 손 위치 (인게임 ImGui 튜닝값) — SetDecoy가 m_v3OffsetPosition에 반영
	Vector3 m_v3OffsetPosFrag  = Vector3{ 5.f, 10.f, 0.f };
	Vector3 m_v3OffsetPosDecoy = Vector3{ 5.f, 1.4f, 0.f };

	std::shared_ptr<IGameObject> m_pModel = nullptr;		// 현재 표시 중인 모델 (아래 캐시 중 하나)
	std::shared_ptr<IGameObject> m_pModelFrag = nullptr;	// 일반 수류탄 (granade)
	std::shared_ptr<IGameObject> m_pModelDecoy = nullptr;	// 디코이 (stun_grenade)
};

// 손 붕대 모델 소켓 — 붕대 들기(4번) 동안만 표시.
// 부착/오프셋/렌더 로직은 수류탄 소켓 그대로, 모델만 bandage.bin
class BandageHandSocket : public GrenadeHandSocket {
public:
	BandageHandSocket(std::shared_ptr<Skeleton> pOwner, int32 nBoneIndex)
		: GrenadeHandSocket{ pOwner, nBoneIndex } {
		m_v3OffsetPosition = Vector3{ 7.7f, 3.5f, 2.0f };	// 인게임 ImGui 튜닝으로 확정한 값
	}
	virtual ~BandageHandSocket() {}

	virtual void Initialize() override;
	virtual const char* SocketLabel() const override { return "Bandage Hand Socket"; }
};


class Skeleton : public IComponent {
	friend class AnimationController;

public:
	Skeleton(std::shared_ptr<IGameObject> pOwner);
	Skeleton(std::shared_ptr<IGameObject> pOwner, std::vector<Bone>& bones);

	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const override;
	void ResetBoneNodes();

	void PrepareRenderAttached();

	const std::vector<Bone>& GetBones() const { return m_Bones; }
	size_t GetRootBoneIndex() const { return m_nRootBoneIndex; }
	int FindBoneIndex(const std::string& strBoneName) const;

	template<typename T> requires std::derived_from<T, IAttachSocket>
	std::shared_ptr<T> CreateAttachSocket(const std::string& strBoneNameToAttach);
	void DetachSocket(std::shared_ptr<IAttachSocket> pAttachable);

	bool IsOwnerHasAnimation() const { return m_pFinalModelLocalRef; }
	const std::vector<Matrix>* TryGetAnimationModelLocalTransforms() const { return m_pFinalModelLocalRef; }

	virtual void ShowControlImGui() override;

private:
	std::vector<Bone>& GetBonesRef() { return m_Bones; }

private:
	std::vector<Bone> m_Bones;
	size_t m_nRootBoneIndex = 0;

	std::vector<std::shared_ptr<IAttachSocket>> m_pAttached;
	const std::vector<Matrix>* m_pFinalModelLocalRef = nullptr;
};

template<typename T> requires std::derived_from<T, IAttachSocket>
inline std::shared_ptr<T> Skeleton::CreateAttachSocket(const std::string& strBoneNameToAttach)
{
	int32 nBoneIdx = FindBoneIndex(strBoneNameToAttach);
	if (nBoneIdx == -1) {
		__debugbreak();
		return nullptr;
	}

	std::shared_ptr<T> pAttachSocket = std::make_shared<T>(static_pointer_cast<Skeleton>(shared_from_this()), nBoneIdx);
	m_pAttached.push_back(pAttachSocket);
	return pAttachSocket;
}

template <>
struct ComponentIndex<Skeleton> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::SKELETON;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::SKELETON);
};
