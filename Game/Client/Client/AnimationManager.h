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

private:
	std::shared_ptr<Animation> LoadFromFile(const std::string& strName);


private:
	std::unordered_map<std::string, std::shared_ptr<Animation>> m_pAnimationMap;

private:
	inline static std::string g_strAnimationBasePath = "../Resources/Animations";

};

