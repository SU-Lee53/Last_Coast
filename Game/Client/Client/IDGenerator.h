#pragma once

template<std::integral T>
class IDGenerator {
public:
	T Generate();
	void Free(T id);

private:
	T m_Base = 0;
	std::vector<T> m_FreeIDs;
};

template<std::integral T>
inline T IDGenerator<T>::Generate()
{
	if (m_FreeIDs.size() != 0) {
		T id = m_FreeIDs.back();
		m_FreeIDs.pop_back;
		return id;
	}

	return m_Base++;
}

template<std::integral T>
inline void IDGenerator<T>::Free(T id)
{
	m_FreeIDs.push_back(id);
}
