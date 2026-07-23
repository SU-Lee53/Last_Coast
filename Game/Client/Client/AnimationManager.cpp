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
	LoadAndAdd("Pistol Idle");
	LoadAndAdd("Firing Rifle");
	LoadAndAdd("Standing Melee Attack Horizontal");
	LoadAndAdd("Reloading");
	LoadAndAdd("Pistol Reloading");
	LoadAndAdd("Pistol Firing");
	LoadAndAdd("Drawing Pistol");	// 무기 교체(꺼내기) 모션 — 권총
	LoadAndAdd("Drawing Rifle");	// 무기 교체(꺼내기) 모션 — 주무기(라이플류)

	LoadAndAdd("Zombie Idle");
	LoadAndAdd("Zombie Running");
	LoadAndAdd("Zombie Attack");
	LoadAndAdd("Zombie Death");

	LoadAndAdd("Gunship_Anim"); // 헬기 추락 컷씬 로터 회전 (HelicopterAnimationController)
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

void AnimationManager::RegisterAnimationController(AnimationController* pAnimCtrl)
{
	if (m_pUpdateQueue.end() == std::find(m_pUpdateQueue.begin(), m_pUpdateQueue.end(), pAnimCtrl)) {
		m_pUpdateQueue.push_back(pAnimCtrl);
	}
}

void AnimationManager::RemoveAnimationController(const AnimationController* pAnimCtrl)
{
	std::erase(m_pUpdateQueue, pAnimCtrl);
}

void AnimationManager::UpdateAnimationParallel()
{
	using namespace std::chrono;
	auto beginTime = high_resolution_clock::now();

	// 100 Updates
	// 180 ~ 220 us
	if (m_pUpdateQueue.size() <= 10) {
		for (uint32 i = 0; i < m_pUpdateQueue.size(); ++i) {
			m_pUpdateQueue[i]->Update();
		}
	}
	else {
		tbb::parallel_for(
			tbb::blocked_range<size_t>(0, m_pUpdateQueue.size()),
			[&](const tbb::blocked_range<size_t>& r) {
				for (size_t i = r.begin(); i != r.end(); ++i) {
					m_pUpdateQueue[i]->Update();
				}
			}
		);
	}


	// 100 Updates
	// 600 ~ 700 us
	//for (uint32 i = 0; i < m_pUpdateQueue.size(); ++i) {
	//	m_pUpdateQueue[i]->Update();
	//}

	auto endTime = high_resolution_clock::now();
	m_llUpdateTime = duration_cast<microseconds>(endTime - beginTime).count();
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

	pAnimation->m_keyFrameMap.Reserve(nChannels);

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
		pAnimation->m_keyFrameMap.Insert(boneName, keyFrames);
	}
	return pAnimation;
}

void AnimationManager::ShowDebugOptions()
{
	ImGui::Begin("AnimationManager");
	{
		ImGui::Text("Anim Controllers in Queue : %d", m_pUpdateQueue.size());
		ImGui::Text("Update time : %lld microseconds", m_llUpdateTime);
	}
	ImGui::End();
}
