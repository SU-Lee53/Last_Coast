#pragma once

class ShaderManager {
public:
	ShaderManager(ComPtr<ID3D12Device> pDevice);
	~ShaderManager();

public:
	void Initialize();

	template<typename T> requires std::derived_from<T, Shader>
	bool Load(ComPtr<ID3D12RootSignature> pRootSignature = RenderManager::g_pd3dGlobalRootSignature);

	template<typename T> requires std::derived_from<T, Shader>
	std::shared_ptr<T> Get();

	D3D12_SHADER_BYTECODE GetShaderByteCode(const std::string& strShaderName);

	void CompileShaders();
	void ReleaseBlobs();


private:
	ComPtr<ID3D12Device>				m_pd3dDevice = nullptr;		// Reference to D3DCore::m_pd3dDevice

private:
	std::unordered_map<std::type_index, std::shared_ptr<Shader>> m_pShaderMap;
	std::unordered_map<std::string, D3D12_SHADER_BYTECODE> m_pCompiledShaderByteCodeMap;

	std::vector<ComPtr<ID3DBlob>> m_pd3dBlobs;
};

template<typename T> requires std::derived_from<T, Shader>
inline bool ShaderManager::Load(ComPtr<ID3D12RootSignature> pRootSignature)
{
	if (!m_pShaderMap.contains(typeid(T))) {
		std::shared_ptr<T> pShader = std::make_shared<T>();
		pShader->Initialize(m_pd3dDevice, pRootSignature);
		m_pShaderMap.insert({ typeid(T), pShader });
		return true;
	}

	return false;
}

template<typename T> requires std::derived_from<T, Shader>
inline std::shared_ptr<T> ShaderManager::Get()
{
	auto it = m_pShaderMap.find(typeid(T));
	if (it != m_pShaderMap.end()) {
		return static_pointer_cast<T>(it->second);
	}

	return nullptr;
}
