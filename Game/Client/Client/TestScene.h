#pragma once
#include "Scene.h"
#include "NavMeshDebugRenderer.h"

class TestScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;
	void Render(ComPtr<ID3D12GraphicsCommandList> pd3dCommansList) override;

private:
	std::unique_ptr<NavMeshDebugRenderer> m_pNavMeshDebugRenderer;
};

