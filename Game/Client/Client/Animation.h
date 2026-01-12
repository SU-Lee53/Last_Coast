#pragma once

struct KeyFrame {
	double fTime;
	AnimationKey animationKeys;
};

class Animation {
	friend class AnimationManager;

public:
	Matrix GetKeyFrameMatrix(const std::string& strChannelName, float fTime, const Matrix& mtxTransformation);	// Channel Name == Bone Name
	AnimationKey GetKeyFrameSRT(const std::string& strChannelName, float fTime, const Matrix& mtxTransformation);	// Channel Name == Bone Name
	
	const std::string& GetName() const { return m_strName; }
	float GetDuration() const { return m_fDuration; }
	float GetTicksForSecond() const { return m_fTicksPerSecond; }

private:
	std::string m_strName;
	float m_fDuration;
	float m_fTicksPerSecond;

	std::unordered_map<std::string, std::vector<KeyFrame>> m_keyFrameMap;
};

