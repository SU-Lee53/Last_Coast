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

private:
	WEAPON_TYPE m_eCurrentWeapon;

	Vector3 m_v3OffsetPosition = Vector3::Zero;
	Vector3 m_v3OffsetRotation = Vector3(180.f, -90.f, -90.f);

	std::shared_ptr<WeaponObject> m_pWeaponModel = nullptr;

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
