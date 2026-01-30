#pragma once
#include "Component.h"

class IGameObject;

class Transform : public IComponent{
public:
	Transform(std::shared_ptr<IGameObject> pOwner);

	virtual void Initialize() override;
	virtual void Update() override;
	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const override;

public:
	void SetFrameMatrix(const Matrix& mtxFrame);

	// 아래 함수는 이번 프로젝트에서 사용
	// 특별한 경우가 아니라면 직접 월드 행렬을 Set하는 일은 없는것이 좋음
	void SetWorldMatrix(const Matrix& mtxWorld);

public:
	void SetPosition(float x, float y, float z);
	void SetPosition(const Vector3& xmf3Position);

	void Move(Vector3 v3MoveDirection, float fAmount);

public:
	// Rotate 이후 이걸 쓰면 기존의 모든 회전이 날아감
	void SetRotation(float fPitch, float fYaw, float fRoll);

	// Rotate 이후 이걸 쓰면 기존의 모든 회전이 날아감
	void SetRotation(const Vector3& xmf3Rotation);

	void SetRotation(const Quaternion& xmf3Rotation);

	void Rotate(const Vector3& v3Rotation);
	void Rotate(float fPitch, float fYaw, float fRoll);
	void Rotate(const Vector3& v3Axis, float fAngle);
	
	void RotateWorld(float fPitch, float fYaw, float fRoll, const Vector3& v3BaseOnWorld = Vector3(0,0,0));
	void RotateWorld(const Vector3& v3Axis, float fAngle, const Vector3& v3BaseOnWorld = Vector3(0, 0, 0));

public:
	void Scale(float fxScale, float fyScale, float fzScale);
	void Scale(const Vector3& v3Scale);
	void Scale(float fScale);

public:
	const Vector3& GetPosition() const;
	const Vector3& GetRight() const;
	const Vector3& GetUp() const;
	const Vector3& GetLook() const;
	
	const Matrix& GetWorldMatrix() const;
	const Matrix& GetFrameMatrix() const;

private:
	// m_mtxFrameRelative : 계층 모델에서 부모로부터의 상대 변환
	// m_mtxTransform : 오브젝트가 수행할 변환의 누적
	// m_mtxWorld : m_mtxTransform * m_mtxFrameRelative (자식이라면 * (부모의 월드))
	//             월드 원점 기준 최종 변환 행렬

	Matrix m_mtxFrameRelative	= {};
	Matrix m_mtxTransform		= {};
	Matrix m_mtxWorld			= {};

	// 외부에서 World 를 Set 한 경우 Update 를 막도록 함
	bool m_bWorldSetFromOutside = false;


};

template <>
struct ComponentIndex<Transform> {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::TRANSFORM;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::TRANSFORM);
};

