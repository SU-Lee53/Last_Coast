#pragma once
#include <random>

class RandomGenerator {
public:
	static int GenerateRandomIntInRange(int min, int max) {
		return std::uniform_int_distribution<int>{ min, max }(g_dre);
	}

	static float GenerateRandomFloatInRange(float min, float max) {
		return std::uniform_real_distribution<float>{min, max}(g_dre);
	}

	static XMFLOAT4 GenerateRandomColor() {
		std::uniform_real_distribution<float> uid{ 0.f, 1.f };
		float r = uid(g_dre);
		float g = uid(g_dre);
		float b = uid(g_dre);

		return XMFLOAT4(r, g, b, 1.0f);
	}

	static XMVECTOR GenerateRandomUnitVectorOnSphere() {
		XMVECTOR xmvOne = XMVectorReplicate(1.0f);
		XMVECTOR xmvZero = XMVectorZero();

		while (true) {
			XMVECTOR v = XMVectorSet(GenerateRandomFloatInRange(-1.f, 1.f), GenerateRandomFloatInRange(-1.f, 1.f), GenerateRandomFloatInRange(-1.f, 1.f), 0.0f);
			if (!XMVector3Greater(XMVector3LengthSq(v), xmvOne)) {
				return XMVector3Normalize(v);
			}
		}

	}

private:
	static std::default_random_engine g_dre;
};

