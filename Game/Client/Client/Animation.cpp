#include "pch.h"
#include "Animation.h"

Matrix Animation::GetKeyFrameMatrix(const std::string& strChannelName, double dTime, const Matrix& mtxTransformation)
{
	auto it = m_keyFrameMap.find(strChannelName);
	if (it == m_keyFrameMap.end()) {
		return mtxTransformation;
	}
	const std::vector<KeyFrame>& keyFrames = it->second;

	if (keyFrames.size() == 0) {
		return mtxTransformation;
	}

	if (keyFrames.size() == 1) {
		return AnimationKey::CreateSRT(keyFrames[0].animationKeys.v3Position, keyFrames[0].animationKeys.v4RotationQuat, keyFrames[0].animationKeys.v3Scale);
	}

	// dTime 이 마지막 키를 지나 처음으로 되돌아옴
	if (dTime >= keyFrames.back().dTime) {
		double dInterval = keyFrames.back().dTime - keyFrames.front().dTime;
		double t = (dTime - keyFrames.back().dTime) / dInterval;

		Vector3 v3Scale0 = keyFrames.back().animationKeys.v3Scale;
		Quaternion v4Rotation0 = keyFrames.back().animationKeys.v4RotationQuat;
		Vector3 v3Translation0 = keyFrames.back().animationKeys.v3Position;
		v4Rotation0.Normalize();

		Vector3 v3Scale1 = keyFrames.front().animationKeys.v3Scale;
		Quaternion v4Rotation1 = keyFrames.front().animationKeys.v4RotationQuat;
		Vector3 v3Translation1 = keyFrames.front().animationKeys.v3Position;
		v4Rotation1.Normalize();

		Vector3 v3Scale = Vector3::Lerp(v3Scale0, v3Scale1, t);
		Quaternion v4Rotation = Quaternion::Slerp(v4Rotation0, v4Rotation1, t);
		Vector3 v3Translation = Vector3::Lerp(v3Translation0, v3Translation1, t);

		return AnimationKey::CreateSRT(v3Translation, v4Rotation, v3Scale);
	}

	auto curIt = std::lower_bound(keyFrames.begin(), keyFrames.end(), dTime, [](const KeyFrame& keyFrame, double dTime) {
		return keyFrame.dTime < dTime;
	});

	if (curIt == keyFrames.begin()) {
		return AnimationKey::CreateSRT(keyFrames[0].animationKeys.v3Position, keyFrames[0].animationKeys.v4RotationQuat, keyFrames[0].animationKeys.v3Scale);
	}

	auto curKeyFrame = *std::prev(curIt);
	auto nextKeyFrame = *curIt;

	double dInterval = nextKeyFrame.dTime - curKeyFrame.dTime;
	double t = (dTime - curKeyFrame.dTime) / dInterval;

	Vector3 v3Scale0 = curKeyFrame.animationKeys.v3Scale;
	Quaternion v4Rotation0 = curKeyFrame.animationKeys.v4RotationQuat;
	Vector3 v3Translation0 = curKeyFrame.animationKeys.v3Position;
	v4Rotation0.Normalize();

	Vector3 v3Scale1 = nextKeyFrame.animationKeys.v3Scale;
	Quaternion v4Rotation1 = nextKeyFrame.animationKeys.v4RotationQuat;
	Vector3 v3Translation1 = nextKeyFrame.animationKeys.v3Position;
	v4Rotation1.Normalize();

	Vector3 v3Scale = Vector3::Lerp(v3Scale0, v3Scale1, t);
	Quaternion v4Rotation = Quaternion::Slerp(v4Rotation0, v4Rotation1, t);
	Vector3 v3Translation = Vector3::Lerp(v3Translation0, v3Translation1, t);

	return AnimationKey::CreateSRT(v3Translation, v4Rotation, v3Scale);

}

AnimationKey Animation::GetKeyFrameSRT(const std::string& strChannelName, double dTime, const Matrix& mtxTransformation)
{
	auto it = m_keyFrameMap.find(strChannelName);
	if (it == m_keyFrameMap.end()) {
		XMVECTOR xmvTranslation;
		XMVECTOR xmvRotation;
		XMVECTOR xmvScale;

		XMMatrixDecompose(&xmvScale, &xmvRotation, &xmvTranslation, mtxTransformation);

		Vector3 v3Translation = xmvTranslation;
		Vector3 v4Rotation = xmvRotation;
		Vector3 v3Scale = xmvScale;

		return { v3Translation, v4Rotation, v3Scale };
	}
	const std::vector<KeyFrame>& keyFrames = it->second;

	if (keyFrames.size() == 0) {
		XMVECTOR xmvTranslation;
		XMVECTOR xmvRotation;
		XMVECTOR xmvScale;

		XMMatrixDecompose(&xmvScale, &xmvRotation, &xmvTranslation, mtxTransformation);

		Vector3 v3Translation = xmvTranslation;
		Vector3 v4Rotation = xmvRotation;
		Vector3 v3Scale = xmvScale;

		return { v3Translation, v4Rotation, v3Scale };
	}

	if (keyFrames.size() == 1) {
		return { keyFrames[0].animationKeys.v3Position, keyFrames[0].animationKeys.v4RotationQuat, keyFrames[0].animationKeys.v3Scale };
	}

	// dTime 이 마지막 키를 지나 처음으로 되돌아옴
	if (dTime >= keyFrames.back().dTime) {
		double dInterval = keyFrames.back().dTime - keyFrames.front().dTime;
		double t = (dTime - keyFrames.back().dTime) / dInterval;

		Vector3 v3Scale0 = keyFrames.back().animationKeys.v3Scale;
		Quaternion v4Rotation0 = keyFrames.back().animationKeys.v4RotationQuat;
		Vector3 v3Translation0 = keyFrames.back().animationKeys.v3Position;
		v4Rotation0.Normalize();

		Vector3 v3Scale1 = keyFrames.front().animationKeys.v3Scale;
		Quaternion v4Rotation1 = keyFrames.front().animationKeys.v4RotationQuat;
		Vector3 v3Translation1 = keyFrames.front().animationKeys.v3Position;
		v4Rotation1.Normalize();

		Vector3 v3Scale = Vector3::Lerp(v3Scale0, v3Scale1, t);
		Quaternion v4Rotation = Quaternion::Slerp(v4Rotation0, v4Rotation1, t);
		Vector3 v3Translation = Vector3::Lerp(v3Translation0, v3Translation1, t);

		return { v3Translation, v4Rotation, v3Scale };
	}

	auto curIt = std::lower_bound(keyFrames.begin(), keyFrames.end(), dTime, [](const KeyFrame& keyFrame, double dTime) {
		return keyFrame.dTime < dTime;
		});

	if (curIt == keyFrames.begin()) {
		return { keyFrames[0].animationKeys.v3Position, keyFrames[0].animationKeys.v4RotationQuat, keyFrames[0].animationKeys.v3Scale };
	}

	auto curKeyFrame = *std::prev(curIt);
	auto nextKeyFrame = *curIt;

	double dInterval = nextKeyFrame.dTime - curKeyFrame.dTime;
	double t = (dTime - curKeyFrame.dTime) / dInterval;

	Vector3 v3Scale0 = curKeyFrame.animationKeys.v3Scale;
	Quaternion v4Rotation0 = curKeyFrame.animationKeys.v4RotationQuat;
	Vector3 v3Translation0 = curKeyFrame.animationKeys.v3Position;
	v4Rotation0.Normalize();

	Vector3 v3Scale1 = nextKeyFrame.animationKeys.v3Scale;
	Quaternion v4Rotation1 = nextKeyFrame.animationKeys.v4RotationQuat;
	Vector3 v3Translation1 = nextKeyFrame.animationKeys.v3Position;
	v4Rotation1.Normalize();

	Vector3 v3Scale = Vector3::Lerp(v3Scale0, v3Scale1, t);
	Quaternion v4Rotation = Quaternion::Slerp(v4Rotation0, v4Rotation1, t);
	Vector3 v3Translation = Vector3::Lerp(v3Translation0, v3Translation1, t);

	return { v3Translation, v4Rotation, v3Scale };

}
