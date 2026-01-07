#pragma once

struct AnimationKey {
	Vector3 v3Position{ 0.f, 0.f, 0.f };
	Quaternion v4RotationQuat{ 0.f, 0.f, 0.f, 1.f };
	Vector3 v3Scale{ 1.f, 1.f, 1.f };

	Matrix CreateSRT() const {
		Matrix mtxScale = Matrix::CreateScale(v3Scale);
		Matrix mtxRotation = Matrix::CreateFromQuaternion(v4RotationQuat);
		Matrix mtxTranslation = Matrix::CreateTranslation(v3Position);
		return mtxScale * mtxRotation * mtxTranslation;
	}

	static Matrix CreateSRT(const Vector3& v3Translation, const Quaternion& v4RotationQuat, const Vector3& v3Scale) {
		Matrix mtxScale = Matrix::CreateScale(v3Scale);
		Matrix mtxRotation = Matrix::CreateFromQuaternion(v4RotationQuat);
		Matrix mtxTranslation = Matrix::CreateTranslation(v3Translation);
		return mtxScale * mtxRotation * mtxTranslation;
	}

	static AnimationKey Lerp(const AnimationKey& key1, const AnimationKey& key2, float fWeight) {
		Vector3 v3Scale = Vector3::Lerp(key1.v3Scale, key2.v3Scale, fWeight);
		Quaternion v4Rotation = Quaternion::Slerp(key1.v4RotationQuat, key2.v4RotationQuat, fWeight);
		Vector3 v3Translation = Vector3::Lerp(key1.v3Position, key2.v3Position, fWeight);

		return { v3Translation, v4Rotation, v3Scale };
	}

	static AnimationKey SmoothStep(const AnimationKey& key1, const AnimationKey& key2, float fWeight) {
		Vector3 v3Scale = Vector3::SmoothStep(key1.v3Scale, key2.v3Scale, fWeight);
		Quaternion v4Rotation = Quaternion::Slerp(key1.v4RotationQuat, key2.v4RotationQuat, fWeight);
		Vector3 v3Translation = Vector3::SmoothStep(key1.v3Position, key2.v3Position, fWeight);

		return { v3Translation, v4Rotation, v3Scale };
	}

};

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

