#include "pch.h"
#include "AnimationController.h"
#include "ThirdPersonPlayer.h"
#include "ThirdPersonCamera.h"
#include "Skeleton.h"

AnimationController::AnimationController(std::shared_ptr<IGameObject> pOwner)
	: IComponent{ pOwner }
{
	m_wpOwnerSkeleton = m_wpOwner.lock()->GetComponent<Skeleton>();
}

void AnimationController::Initialize()
{
}

void AnimationController::Update()
{
	m_fTotalTimeElapsed += DT;
	ProcessInput();

	if (m_pStateMachine) {
		m_pStateMachine->Update();
	}

	if (m_pAnimationMontage) {
		m_pAnimationMontage->Update();
	}

	ComputeAnimation();
	ComputeFinalMatrix();
}

const std::vector<Matrix> AnimationController::GetFinalOutput() const
{
	return m_mtxFinalBoneTransforms;
}

double AnimationController::GetCurrentAnimationDuration() const
{
	return m_pStateMachine->GetCurrentAnimationState()->pAnimationToPlay->GetDuration();
}

void AnimationController::ComputeFinalMatrix()
{
	const auto& ownerBones = GetOwnerBones();
	int nBones = ownerBones.size();
	std::vector<Matrix> mtxToRootTransforms(nBones);
	for (int i = 0; i < nBones; ++i) {
		int nParentIndex = ownerBones[i].nParentIndex;
		Matrix mtxToRoot = nParentIndex >= 0 ? m_mtxCachedLocalBoneTransforms[i] * mtxToRootTransforms[nParentIndex] : m_mtxCachedLocalBoneTransforms[i];
		mtxToRootTransforms[i] = mtxToRoot;
	}

	m_mtxFinalBoneTransforms.resize(nBones);
	for (int i = 0; i < nBones; ++i) {
		m_mtxFinalBoneTransforms[i] = ownerBones[i].mtxOffset * mtxToRootTransforms[i];
		m_mtxFinalBoneTransforms[i] = m_mtxFinalBoneTransforms[i].Transpose();
	}
}

void AnimationController::CacheAnimationKey(const std::string& strAnimationName)
{
	const auto& ownerBones = GetOwnerBones();
	m_mtxCachedPose.resize(ownerBones.size());
	const auto& pAnimation = ANIMATION->Get(strAnimationName);
	double fTime = std::fmod(m_fTotalTimeElapsed, pAnimation->GetDuration());

	for (const auto& bone : ownerBones) {
		m_mtxCachedPose[bone.nIndex] = pAnimation->GetKeyFrameSRT(bone.strBoneName, fTime, bone.mtxTransform);
	}
}

const std::vector<Bone>& AnimationController::GetOwnerBones() const
{
	return m_wpOwnerSkeleton.lock()->GetBones();
}

PlayerAnimationController::PlayerAnimationController(std::shared_ptr<IGameObject> pOwner)
	: AnimationController{ pOwner }
{
}

void PlayerAnimationController::Initialize()
{
	m_pStateMachine = std::make_unique<PlayerAnimationStateMachine>();
	m_pStateMachine->Initialize(m_wpOwner.lock(), m_fTotalTimeElapsed);

	m_pAnimationMontage = std::make_unique<PlayerAnimationMontage>();
	m_pAnimationMontage->Initialize(m_wpOwner.lock());

	int nBones = GetOwnerBones().size();
	m_mtxCachedLocalBoneTransforms.resize(nBones);
	m_mtxFinalBoneTransforms.resize(nBones);

	m_pBlendMachine = std::make_unique<LayeredBlendMachine>(m_wpOwner.lock(), "Spine", 3);
	m_nSpineIndex = m_wpOwnerSkeleton.lock()->FindBoneIndex("Spine");

	const auto& pCamera = std::static_pointer_cast<ThirdPersonPlayer>(m_wpOwner.lock())->GetCamera();
	m_wpPlayerCamera = std::static_pointer_cast<ThirdPersonCamera>(pCamera);
}

void PlayerAnimationController::ComputeAnimation()
{
	const std::vector<Bone>& ownerBones = GetOwnerBones();
	const std::vector<AnimationKey>& basePose = m_pStateMachine->GetOutputPose();
	float fMontageBlendWeight = m_pAnimationMontage->GetBlendWeight();
	
	if (fMontageBlendWeight > 0.f) {
		const std::vector<AnimationKey>& blendPose = m_pAnimationMontage->GetOutputPose();
		m_pBlendMachine->Blend(ownerBones, basePose, blendPose, m_mtxCachedLocalBoneTransforms, fMontageBlendWeight);

		// 조준 허리 회전
		float fCameraPitch = m_wpPlayerCamera.lock()->GetPitch();
		Quaternion v4CameraPitch = Quaternion::CreateFromYawPitchRoll(0.f, 0.f, -XMConvertToRadians(fCameraPitch));

		std::vector<Matrix> mtxComponentSpace;
		AnimationHepler::LocalToComponent(ownerBones, m_mtxCachedLocalBoneTransforms, mtxComponentSpace);
		AnimationHepler::TransformModifyBone(ownerBones, m_nSpineIndex, m_mtxCachedLocalBoneTransforms, mtxComponentSpace, v4CameraPitch, fMontageBlendWeight);
		AnimationHepler::ComponentToLocal(ownerBones, mtxComponentSpace, m_mtxCachedLocalBoneTransforms);
	}
	else {
		m_mtxCachedLocalBoneTransforms.resize(ownerBones.size());
		for (int i = 0; i < basePose.size(); ++i) {
			m_mtxCachedLocalBoneTransforms[i] = basePose[i].CreateSRT();
		}
	}
}

std::shared_ptr<IComponent> PlayerAnimationController::Copy(std::shared_ptr<IGameObject> pNewOwner)const
{
	std::shared_ptr<PlayerAnimationController> pClone = std::make_shared<PlayerAnimationController>(pNewOwner);
	pClone->Initialize();

	return pClone;
}
