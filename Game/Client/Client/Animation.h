#pragma once

struct AnimationKey {
	Vector3 v3Position{ 0.f, 0.f, 0.f };
	Quaternion v4RotationQuat{ 0.f, 0.f, 0.f, 1.f };
	Vector3 v3Scale{ 1.f, 1.f, 1.f };
};

struct KeyFrame {
	double dTime;
	AnimationKey animationKeys;
};

class Animation {
	friend class AnimationManager;

public:
	Matrix GetSRT(const std::string& strChannelName, double dTime, const Matrix& mtxTransformation);	// Channel Name == Bone Name
	
	const std::string& GetName() const { return m_strName; }
	double GetDuration() const { return m_dDuration; }
	double GetTicksForSecond() const { return m_dTicksPerSecond; }

private:
	std::string m_strName;
	double m_dDuration;
	double m_dTicksPerSecond;

	std::unordered_map<std::string, std::vector<KeyFrame>> m_keyFrameMap;


};

