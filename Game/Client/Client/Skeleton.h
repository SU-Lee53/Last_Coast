#pragma once
#include "Component.h"

class Skeleton : public IComponent {
	friend class AnimationController;

public:
	Skeleton(std::shared_ptr<IGameObject> pOwner);
	Skeleton(std::shared_ptr<IGameObject> pOwner, std::vector<Bone>& bones);

	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner)const override;

	const std::vector<Bone>& GetBones() const { return m_Bones; }
	size_t GetRootBoneIndex() const { return m_nRootBoneIndex; }
	int FindBoneIndex(const std::string& strBoneName) const;

private:
	std::vector<Bone>& GetBonesRef() { return m_Bones; }

private:
	std::vector<Bone> m_Bones;
	size_t m_nRootBoneIndex = 0;

};

template <>
struct ComponentIndex<Skeleton> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::SKELETON;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::SKELETON);
};
