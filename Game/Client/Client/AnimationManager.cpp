#include "pch.h"
#include "AnimationManager.h"

void AnimationManager::Initialize()
{
	LoadGameAnimations();
}

void AnimationManager::LoadGameAnimations()
{
	LoadAndAdd("Breathing Idle");
	LoadAndAdd("Walking");
	LoadAndAdd("Jog Forward");
	LoadAndAdd("Rifle Aiming Idle");
	LoadAndAdd("Firing Rifle");
	LoadAndAdd("Zombie Attack");
	LoadAndAdd("Zombie Death");
}

std::shared_ptr<Animation> AnimationManager::LoadAndAdd(const std::string& strName)
{
	auto it = m_pAnimationMap.find(strName);
	if (it != m_pAnimationMap.end()) {
		return it->second;
	}

	std::shared_ptr<Animation> pAnimation = LoadFromFile(strName);
	m_pAnimationMap.insert({ strName, pAnimation });
	
	return pAnimation;
}

std::shared_ptr<Animation> AnimationManager::Add(const std::string& strName, std::shared_ptr<Animation> pAnimation)
{
	auto it = m_pAnimationMap.find(strName);
	if (it != m_pAnimationMap.end()) {
		return it->second;
	}
	m_pAnimationMap.insert({ strName, pAnimation });

	return pAnimation;
}

std::shared_ptr<Animation> AnimationManager::Get(const std::string& strName)
{
	auto it = m_pAnimationMap.find(strName);
	if (it == m_pAnimationMap.end()) {
		return nullptr;
	}

	return it->second;
}

std::shared_ptr<Animation> AnimationManager::LoadFromFile(const std::string& strName)
{
	namespace fs = std::filesystem;

	std::string strPath = std::format("{}/{}.bin", g_strAnimationBasePath, strName);

	std::ifstream in{ strPath, std::ios::binary };
	if (!in) {
		__debugbreak();
		return nullptr;
	}

	auto buf = ::ReadBinaryFile(strPath);
	nlohmann::json j = nlohmann::json::from_bson(buf);

	std::shared_ptr<Animation> pAnimation = std::make_shared<Animation>();

	const nlohmann::json& jAnimation = j["Animations"][0];

	pAnimation->m_strName = jAnimation["Name"].get<std::string>();
	pAnimation->m_fDuration = jAnimation["Duration"].get<double>();
	pAnimation->m_fTicksPerSecond = jAnimation["TicksPerSecond"].get<double>();

	unsigned nChannels = jAnimation["nChannels"].get<unsigned>();
	const nlohmann::json& jChannels = jAnimation["Channels"];

	if constexpr (requires{ pAnimation->m_keyFrameMap.reserve(nChannels); }) {
		pAnimation->m_keyFrameMap.reserve(nChannels);
	}

	for (uint32 i = 0; i < nChannels; ++i) {
		const nlohmann::json& jChannel = jChannels[i];
		const size_t nKeyframes = jChannel["nKeyFrames"].get<size_t>();
		const nlohmann::json& jKeyframes = jChannel["KeyFrames"];

		std::vector<KeyFrame> keyFrames;
		keyFrames.reserve(nKeyframes);
		for (uint32 keyIndex = 0; keyIndex < nKeyframes; ++keyIndex) {
			const nlohmann::json& jKey = jKeyframes[keyIndex];

			const double dTime = jKey[0].get<float>();
			const Vector3 t = ReadVector3FromJson(jKey[1]);
			const Quaternion r = ReadVector4FromJson(jKey[2]);
			const Vector3 s = ReadVector3FromJson(jKey[3]);
			
			keyFrames.emplace_back(dTime, t, r, s);
		}


		std::string boneName = jChannels[i]["Name"].get<std::string>();
		pAnimation->m_keyFrameMap.emplace(boneName, keyFrames);
	}


	//for (int i = 0; i < nChannels; ++i) {
	//	std::vector<KeyFrame> keyFrames;
	//	size_t nKeyFrames = jChannels[i]["nKeyFrames"].get<size_t>();
	//	keyFrames.reserve(nKeyFrames);

	//	const nlohmann::json& jKeyFrames = jChannels[i]["KeyFrames"];
	//	for (int keyIndex = 0; keyIndex < nKeyFrames; ++keyIndex) {
	//		KeyFrame k;
	//		//k.dTime = jChannels["KeyFrames"];
	//		k.fTime = jKeyFrames[keyIndex][0].get<double>();

	//		std::vector<float> keyData = jKeyFrames[keyIndex][1].get<std::vector<float>>();
	//		k.animationKeys.v3Translation = Vector3(keyData.data());
	//		
	//		keyData = jKeyFrames[keyIndex][2].get<std::vector<float>>();
	//		k.animationKeys.v4RotationQuat = Vector4(keyData.data());
	//		
	//		keyData = jKeyFrames[keyIndex][3].get<std::vector<float>>();
	//		k.animationKeys.v3Scale = Vector3(keyData.data());

	//		keyFrames.push_back(k);
	//	}

	//	std::string boneName = jChannels[i]["Name"].get<std::string>();
	//	pAnimation->m_keyFrameMap.insert({ boneName, keyFrames });
	//}

	return pAnimation;
}
