#pragma once

template<typename T>
class TypedObjectPool {
private:
	struct Slot {
		Slot(std::shared_ptr<T> pObj) : pObj{ std::move(pObj) }, bAlive{true} {}
		std::shared_ptr<T> pObj;
		bool bAlive;
	};

public:
	template<bool IsConst>
	class AliveIterator {
	private:
		using PoolVector = std::vector<Slot>;
		using InnerIterator = std::conditional_t<
			IsConst,
			typename PoolVector::const_iterator,
			typename PoolVector::iterator
		>;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = std::shared_ptr<T>;
		using difference_type = std::ptrdiff_t;
		using reference = std::conditional_t<
			IsConst,
			const std::shared_ptr<T>&,
			std::shared_ptr<T>&
		>;
		using pointer = std::conditional_t<
			IsConst,
			const std::shared_ptr<T>*,
			std::shared_ptr<T>*
		>;

	public:
		AliveIterator() = default;

		AliveIterator(InnerIterator it, InnerIterator end)
			: m_it{ it }
			, m_end{ end } {
			SkipDeadSlots();
		}

		reference operator*() const {
			return m_it->pObj;
		}

		pointer operator->() const {
			return &m_it->pObj;
		}

		AliveIterator& operator++() {
			++m_it;
			SkipDeadSlots();
			return *this;
		}

		AliveIterator operator++(int) {
			AliveIterator temp = *this;
			++(*this);
			return temp;
		}

		friend bool operator==(const AliveIterator& lhs, const AliveIterator& rhs) {
			return lhs.m_it == rhs.m_it;
		}

		friend bool operator!=(const AliveIterator& lhs, const AliveIterator& rhs) {
			return !(lhs == rhs);
		}

	private:
		void SkipDeadSlots() {
			while (m_it != m_end) {
				if (m_it->bAlive && m_it->pObj) {
					break;
				}

				++m_it;
			}
		}

	private:
		InnerIterator m_it{};
		InnerIterator m_end{};
	};

public:
	using iterator = AliveIterator<false>;
	using const_iterator = AliveIterator<true>;

	iterator begin() {
		return iterator{ m_Pool.begin(), m_Pool.end() };
	}

	iterator end() {
		return iterator{ m_Pool.end(), m_Pool.end() };
	}

	const_iterator begin() const {
		return const_iterator{ m_Pool.begin(), m_Pool.end() };
	}

	const_iterator end() const {
		return const_iterator{ m_Pool.end(), m_Pool.end() };
	}

	const_iterator cbegin() const {
		return const_iterator{ m_Pool.cbegin(), m_Pool.cend() };
	}

	const_iterator cend() const {
		return const_iterator{ m_Pool.cend(), m_Pool.cend() };
	}



public:
	void Reserve(size_t nNewSize) {
		m_Pool.reserve(nNewSize);
	}

	void Add(const std::shared_ptr<T>& pObj) {
		if (!pObj) {
			return;
		}

		++m_nSize;

		if (!m_freeIndex.empty()) {
			size_t idx = m_freeIndex.front();
			m_freeIndex.pop();
			m_Pool[idx].pObj = pObj;
			m_Pool[idx].bAlive = true;
			return;
		}

		m_Pool.emplace_back(pObj);
	}

	void Remove(const std::shared_ptr<T>& pObj) {
		auto it = std::find_if(m_Pool.begin(), m_Pool.end(), [&pObj](const auto& slot) { return (slot.bAlive) && (slot.pObj == pObj); });
		if (it != m_Pool.end()) {
			--m_nSize;
			it->bAlive = false;
			size_t idx = static_cast<size_t>(std::distance(m_Pool.begin(), it));
			m_freeIndex.push(idx);
		}
	}

	void RemoveAndReset(const std::shared_ptr<T>& pObj) {
		auto it = std::find_if(m_Pool.begin(), m_Pool.end(), [&pObj](const auto& slot) { return (slot.bAlive) && (slot.pObj == pObj); });
		if (it != m_Pool.end()) {
			--m_nSize;
			it->pObj.reset();
			it->bAlive = false;
			size_t idx = static_cast<size_t>(std::distance(m_Pool.begin(), it));
			m_freeIndex.push(idx);
		}
	}

	bool Contains(const std::shared_ptr<T>& pObj) const {
		auto it = std::find_if(m_Pool.begin(), m_Pool.end(), [&pObj](const auto& slot) { return (slot.bAlive) && (slot.pObj == pObj); });
		return it != m_Pool.end();
	}

	void Clear() {
		m_nSize = 0;
		m_Pool.clear();
		m_freeIndex.swap(std::queue<size_t>{});
	}

	size_t Size() const {
		return m_nSize;
	}

public:
	// 모든 pObj 에 함수를 적용
	template<typename Func, typename... Args>
	void ForEachAlive(Func&& func, Args&&... args) {
		for (auto& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				std::invoke(func, slot.pObj, args...);
			}
		}
	}

	template<typename Func, typename... Args>
	void ForEachAlive(Func&& func, Args&&... args) const {
		for (const auto& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				std::invoke(func, slot.pObj, args...);
			}
		}
	}

	// 모든 pObj 에 함수를 적용하고, 리턴결과을 모아 std::vector 로 리턴
	template<typename Func, typename... Args>
	auto CollectAlive(Func&& func, Args&&... args) {
		using ReturnType = std::invoke_result_t<Func&, std::shared_ptr<T>&, Args&...>;
		static_assert(!std::is_void_v<ReturnType>, "CollectAlive requires non-void return type.");

		std::vector<std::remove_cvref_t<ReturnType>> results;

		for (Slot& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				results.emplace_back(std::invoke(func, slot.pObj, args...));
			}
		}

		return results;
	}

	template<typename Func, typename... Args>
	auto CollectAlive(Func&& func, Args&&... args) const {
		using ReturnType = std::invoke_result_t<Func&, const std::shared_ptr<T>&, Args&...>;
		static_assert(!std::is_void_v<ReturnType>, "CollectAlive requires non-void return type.");

		std::vector<std::remove_cvref_t<ReturnType>> results;

		for (const Slot& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				results.emplace_back(std::invoke(func, slot.pObj, args...));
			}
		}

		return results;
	}

	// 모든 pObj 에 binary pred 를 적용 (std::any_of)
	template<typename Pred, typename... Args>
	bool AnyOfAlive(Pred&& pred, Args&&... args) const {
		for (const Slot& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				if (std::invoke(pred, slot.pObj, args...)) {
					return true;
				}
			}
		}

		return false;
	}

	// 모든 pObj 에 binary pred 를 적용 (std::all_of)
	template<typename Pred, typename... Args>
	bool AllOfAlive(Pred&& pred, Args&&... args) const {
		for (const Slot& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				if (!std::invoke(pred, slot.pObj, args...)) {
					return false;
				}
			}
		}

		return true;
	}

	// 특정 조건을 만족하는 pObj 를 탐색
	template<typename Pred, typename... Args>
	std::shared_ptr<T> FindIfAlive(Pred&& pred, Args&&... args) const {
		for (const Slot& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				if (std::invoke(pred, slot.pObj, args...)) {
					return slot.pObj;
				}
			}
		}

		return nullptr;
	}

	template<typename Pred, typename... Args>
	std::vector<std::shared_ptr<const T>> FindAllIfAlive(Pred&& pred, Args&&... args) const {
		std::vector<std::shared_ptr<const T>> results;

		for (const Slot& slot : m_Pool) {
			if (slot.bAlive && slot.pObj) {
				if (std::invoke(pred, slot.pObj, args...)) {
					results.push_back(slot.pObj);
				}
			}
		}

		return results;
	}


	template<typename Pred, typename... Args>
	size_t RemoveIfAlive(Pred&& pred, Args&&... args) {
		size_t removed = 0;

		for (size_t i = 0; i < m_Pool.size(); ++i) {
			Slot& slot = m_Pool[i];

			if (slot.bAlive && slot.pObj) {
				if (std::invoke(pred, slot.pObj, args...)) {
					slot.bAlive = false;
					m_freeIndex.push(i);

					--m_nSize;
					++removed;
				}
			}
		}

		return removed;
	}

	template<typename Pred, typename... Args>
	size_t RemoveAndResetIfAlive(Pred&& pred, Args&&... args) {
		size_t removed = 0;

		for (size_t i = 0; i < m_Pool.size(); ++i) {
			Slot& slot = m_Pool[i];

			if (slot.bAlive && slot.pObj) {
				if (std::invoke(pred, slot.pObj, args...)) {
					slot.bAlive = false;
					slot.pObj.reset();
					m_freeIndex.push(i);

					--m_nSize;
					++removed;
				}
			}
		}

		return removed;
	}



private:
	std::vector<Slot> m_Pool;
	std::queue<size_t> m_freeIndex;
	size_t m_nSize = 0;

};


