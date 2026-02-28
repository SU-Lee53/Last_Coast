#pragma once

template<std::integral T>
class IDGenerator {
public:
	T Generate();
	void Free(T id);

private:
	T m_Base = 0;
	std::set<T> m_FreeIndex;
};

template<std::integral T>
inline T IDGenerator<T>::Generate()
{
	if (m_FreeIndex.size() != 0) {
		T ret = *m_FreeIndex.begin();
		m_FreeIndex.erase(m_FreeIndex.begin());
		return ret;
	}

	return m_Base++;
}

template<std::integral T>
inline void IDGenerator<T>::Free(T id)
{
	if (id >= m_Base) {
		--m_Base;
	}
	else {
		m_FreeIndex.insert(id);
	}
}
