#include "pch.h"
#include "Animation.h"

Matrix Animation::GetSRT(const std::string& strChannelName, double dTime, const Matrix& mtxTransformation)
{
	auto it = m_keyFrameMap.find(strChannelName);
	if (it == m_keyFrameMap.end()) {
		return mtxTransformation;
	}
	const std::vector<KeyFrame>& keyFrames = it->second;

	if (keyFrames.size() == 0) {
		return mtxTransformation;
	}

	Matrix mtxSRT;

	if (keyFrames.size() == 1) {
		Matrix mtxScale = Matrix::CreateScale(keyFrames[0].animationKeys.v3Scale);
		Matrix mtxRotation = Matrix::CreateFromQuaternion(XMQuaternionNormalize(keyFrames[0].animationKeys.v4RotationQuat));
		Matrix mtxTranslation = Matrix::CreateTranslation(keyFrames[0].animationKeys.v3Position);
		mtxSRT = mtxScale * mtxRotation * mtxTranslation;

		return mtxSRT;
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
		Quaternion v3Rotation = Quaternion::Slerp(v4Rotation0, v4Rotation1, t);
		Vector3 v3Translation = Vector3::Lerp(v3Translation0, v3Translation1, t);

		Matrix mtxScale = Matrix::CreateScale(v3Scale);
		Matrix mtxRotation = Matrix::CreateFromQuaternion(v3Rotation);
		Matrix mtxTranslation = Matrix::CreateTranslation(v3Translation);
		mtxSRT = mtxScale * mtxRotation * mtxTranslation;

		return mtxSRT;
	}

	auto curIt = std::lower_bound(keyFrames.begin(), keyFrames.end(), dTime, [](const KeyFrame& keyFrame, double dTime) {
		return keyFrame.dTime < dTime;
	});

	if (curIt == keyFrames.begin()) {
		Matrix mtxScale = Matrix::CreateScale(keyFrames[0].animationKeys.v3Scale);
		Matrix mtxRotation = Matrix::CreateFromQuaternion(XMQuaternionNormalize(keyFrames[0].animationKeys.v4RotationQuat));
		Matrix mtxTranslation = Matrix::CreateTranslation(keyFrames[0].animationKeys.v3Position);
		mtxSRT = mtxScale * mtxRotation * mtxTranslation;

		return mtxSRT;
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
	Quaternion v3Rotation = Quaternion::Slerp(v4Rotation0, v4Rotation1, t);
	Vector3 v3Translation = Vector3::Lerp(v3Translation0, v3Translation1, t);

	Matrix mtxScale = Matrix::CreateScale(v3Scale);
	Matrix mtxRotation = Matrix::CreateFromQuaternion(v3Rotation);
	Matrix mtxTranslation = Matrix::CreateTranslation(v3Translation);
	mtxSRT = mtxScale * mtxRotation * mtxTranslation;

	return mtxSRT;

}
