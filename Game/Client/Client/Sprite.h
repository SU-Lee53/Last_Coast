#pragma once
#include "UIComponent.h"

interface IImageSprite abstract{
public:
	IImageSprite(const std::string & strTexturePath);

protected:
	TextureRef<Texture> m_TextureHandle{};

};

class ImageBox : public IUIComponent, public IImageSprite {
public:
	ImageBox(const std::string& strTexturePath) : IImageSprite{ strTexturePath } {
		m_v4Color = Vector4(1.f, 1.f, 1.f, 1.f);
		m_strName = std::filesystem::path{ strTexturePath }.filename().wstring();
	}
	virtual const TextureRef<Texture>& GetTextureRef() const override { return m_TextureHandle; }
	virtual UIRectData MakeSBData() const override;

	virtual void ShowControllImGui() override;
};

class ImageButton : public IUIButtonComponent, public IImageSprite {
public:
	ImageButton(const std::string& strTexturePath) : IImageSprite{ strTexturePath } {
		m_strName = std::filesystem::path{ strTexturePath }.filename().wstring();
	}
	virtual const TextureRef<Texture>& GetTextureRef() const override { return m_TextureHandle; }
	virtual UIRectData MakeSBData() const override;

	virtual void ShowControllImGui() override;
};

class Crosshair : public ImageBox {
public:
	Crosshair() : ImageBox("Crosshair") {
		m_v2Anchor = Vector2{ 0,0 };
		m_v2Pivot = Vector2{0.5f, 0.5f};
		m_v2Position = Vector2{
			static_cast<float>(WinCore::g_dwClientWidth) / 2.0f,
			static_cast<float>(WinCore::g_dwClientHeight) / 2.0f
		};
		m_v2Size = Vector2{ m_fDefaultSize, m_fDefaultSize };

		m_unLayer = 0;
		m_bVisible = false;
	}

	void AddRecoil(float fRecoil) {
		Vector2 v2NewSize = m_v2Size + Vector2(fRecoil);
		if (v2NewSize.x < m_fMaxSize) {
			m_v2Size = v2NewSize;
		}
		else {
			m_v2Size = Vector2(m_fMaxSize);
		}
	}
	void RemoveRecoil(float fRecoil) {
		Vector2 v2NewSize = m_v2Size - Vector2(fRecoil * 10 * DT);
		if (v2NewSize.x > m_fDefaultSize) {
			m_v2Size = v2NewSize;
		}
		else {
			m_v2Size = Vector2(m_fDefaultSize);
		}
	}

private:
	const float m_fDefaultSize = 150.f;
	const float m_fMaxSize = 250.f;


};

class HitMarker : public ImageBox {
public:
	HitMarker() : ImageBox("Hitmarker") {
		m_v2Anchor = Vector2{ 0,0 };
		m_v2Pivot = Vector2{ 0.5f, 0.5f };
		m_v2Position = Vector2{
			static_cast<float>(WinCore::g_dwClientWidth) / 2.0f,
			static_cast<float>(WinCore::g_dwClientHeight) / 2.0f
		};
		m_v2Size = Vector2{ 30, 30 };

		m_unLayer = 0;
		m_bVisible = false;
	}

	void Show() {
		m_fElapsedTime = 0.f;
		m_bVisible = true;
	}

	void Update() {
		if (m_bVisible == false) return;
		if (m_fElapsedTime >= m_fShowTime) {
			m_bVisible = false;
			return;
		}
		m_fElapsedTime += DT;
	}

private:
	float m_fElapsedTime = 0.f;
	const float m_fShowTime = 0.1f;
};
