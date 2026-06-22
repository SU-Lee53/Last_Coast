#pragma once

struct KeyFrame {
	double fTime;
	AnimationKey animationKeys;

	KeyFrame() = default;
	KeyFrame(double fTime, const Vector3& v3Translate, const Quaternion& v4Rotate, const Vector3& v3Scale)
		: fTime{ fTime }, animationKeys{ v3Translate, v4Rotate, v3Scale } { }
};

class Animation {
	friend class AnimationManager;

public:
	Matrix GetKeyFrameMatrix(const std::string& strChannelName, float fTime, const Matrix& mtxTransformation);	// Channel Name == Bone Name
	AnimationKey GetKeyFrameSRT(const std::string& strChannelName, float fTime, const Matrix& mtxTransformation);	// Channel Name == Bone Name
	AnimationKey GetKeyFrameSRT(size_t nChannelIndex, float fTime, const Matrix& mtxTransformation);
	size_t GetChannelIndex(const std::string& strChannelName) const;
	
	const std::string& GetName() const { return m_strName; }
	float GetDuration() const { return m_fDuration; }
	float GetTicksForSecond() const { return m_fTicksPerSecond; }

	float GetLoopedAnimationTime(float fTime) {
		if (m_fDuration <= 0.0f) {
			return 0.0f;
		}

		return std::fmod(fTime, m_fDuration);
	}

private:
	std::string m_strName;
	float m_fDuration;
	float m_fTicksPerSecond;

	IndexMap<std::string, std::vector<KeyFrame>> m_keyFrameMap;
};

