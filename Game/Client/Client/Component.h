#pragma once

class GameObject;

class Component {
public:
	Component(std::shared_ptr<GameObject> pOwner);

	virtual void Initialize () = 0;
	virtual void Update() = 0;

	std::shared_ptr<GameObject> GetOwner() {
		return m_wpOwner.lock();
	}

	void SetOwner(std::shared_ptr<GameObject> pOwner) {
		m_wpOwner = pOwner;
	}

	bool IsOwnerExpired() {
		return m_wpOwner.expired();
	}

	virtual std::shared_ptr<Component> Copy() = 0;

protected:
	std::weak_ptr<GameObject> m_wpOwner;

};

template<typename C>
concept ComponentType = std::derived_from<C, Component>;

template <ComponentType T>
struct ComponentIndex {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE_BASE;
};

template<UINT nIndex>
struct ComponentIndexToType {
	using Type = void;
};

template<>
struct ComponentIndexToType<COMPONENT_TYPE_BASE>
{
	using Type = Component;
};
