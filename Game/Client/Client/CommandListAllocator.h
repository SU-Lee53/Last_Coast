#pragma once

class CommandListAllocator {
public:
	struct CommandListPair {
		uint32 id;
		ComPtr<ID3D12GraphicsCommandList> pd3dCommandList;
		ComPtr<ID3D12CommandAllocator> pd3dCommandAllocator;
	};

public:
	void Initialize(int nCmdLists) {
		m_unCmdCount = nCmdLists;
		HRESULT hr;

		m_CmdPairs.resize(nCmdLists);
		for (int i = 0; i < nCmdLists; ++i) {
			m_CmdPairs[i].id = i;

			hr = DEVICE->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(m_CmdPairs[i].pd3dCommandAllocator.GetAddressOf())
			);

			if (FAILED(hr)) {
				SHOW_ERROR("Failed to create CommandAllocator");
			}

			// Create Command List
			hr = DEVICE->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				m_CmdPairs[i].pd3dCommandAllocator.Get(), NULL,
				IID_PPV_ARGS(m_CmdPairs[i].pd3dCommandList.GetAddressOf())
			);

			if (FAILED(hr)) {
				SHOW_ERROR("Failed to create CommandList");
			}

			// Close Command List(default is opened)
			hr = m_CmdPairs[i].pd3dCommandList->Close();
		}

		m_un64LastFenceValues.resize(nCmdLists, 0);
	}

	CommandListPair& Allocate() {
		HRESULT hr;

		auto& cmdPair = m_CmdPairs[m_unAllocated++];

		hr = cmdPair.pd3dCommandAllocator->Reset();
		if (FAILED(hr)) {
			SHOW_ERROR("Faied to reset CommandAllocator");
			__debugbreak();
		}

		hr = cmdPair.pd3dCommandList->Reset(cmdPair.pd3dCommandAllocator.Get(), nullptr);
		if (FAILED(hr)) {
			SHOW_ERROR("Faied to reset CommandList");
			__debugbreak();
		}

		return cmdPair;
	}

	CommandListPair* AllocateSafe(uint64 un64CompletedFenceValue) {
		HRESULT hr;

		for (uint32 i = 0; i < m_unCmdCount; ++i) {
			if (m_un64LastFenceValues[i] > un64CompletedFenceValue) {
				continue;
			}

			auto& cmdPair = m_CmdPairs[i];
			hr = cmdPair.pd3dCommandAllocator->Reset();
			if (FAILED(hr)) {
				SHOW_ERROR("Faied to reset CommandAllocator");
				__debugbreak();
				return nullptr;
			}

			hr = cmdPair.pd3dCommandList->Reset(cmdPair.pd3dCommandAllocator.Get(), nullptr);
			if (FAILED(hr)) {
				SHOW_ERROR("Faied to reset CommandList");
				__debugbreak();
			}

			return &cmdPair;
		}

		return nullptr;
	}

	void MarkSubmitted(uint32 id, uint64 fenceValue) {
		m_un64LastFenceValues[id] = fenceValue;
	}

	void Reset() {
		m_unAllocated = 0;
	}

	void Shutdown() {
		m_CmdPairs.clear();
		m_un64LastFenceValues.clear();
		m_unCmdCount = 0;
		m_unAllocated = 0;
	}

private:
	std::vector<CommandListPair> m_CmdPairs;

	uint32 m_unCmdCount = 0;
	uint32 m_unAllocated = 0;
	std::vector<uint64> m_un64LastFenceValues;
};

