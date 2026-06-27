#pragma once
#include "Animation.h"

class AnimationManager {

	DECLARE_SINGLE(AnimationManager)

public:
	void Initialize();
	void LoadGameAnimations();

	std::shared_ptr<Animation> LoadAndAdd(const std::string& strName);
	std::shared_ptr<Animation> Add(const std::string& strName, std::shared_ptr<Animation> pAnimation);
	std::shared_ptr<Animation> Get(const std::string& strName);

public:
	void RegisterAnimationController(AnimationController* pAnimCtrl);
	void RemoveAnimationController(const AnimationController* pAnimCtrl);
	void UpdateAnimationParallel();

	void ShowDebugOptions();

private:
	std::shared_ptr<Animation> LoadFromFile(const std::string& strName);

private:
	std::unordered_map<std::string, std::shared_ptr<Animation>> m_pAnimationMap;
	std::vector<AnimationController*> m_pUpdateQueue;

	long long m_llUpdateTime = 0.f;


private:
	inline static std::string g_strAnimationBasePath = "../Resources/Animations";

};

