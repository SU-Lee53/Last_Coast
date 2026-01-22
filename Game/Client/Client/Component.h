#pragma once

class GameObject;

enum class COMPONENT_TYPE : uint8 {
	TRANSFORM = 0,
	MESH_RENDERER,
	ANIMATION_CONTROLLER,

	COUNT,

	BASE = std::numeric_limits<uint8>::max()
};

interface IComponent abstract : std::enable_shared_from_this<IComponent> {
public:
	IComponent(std::shared_ptr<GameObject> pOwner) : m_wpOwner {pOwner} {}

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

	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<GameObject> pNewOwner) = 0;

protected:
	std::weak_ptr<GameObject> m_wpOwner;
	bool m_bInitialized = false;

};

template<typename C>
concept ComponentType = std::derived_from<C, IComponent>;

template <ComponentType T>
struct ComponentIndex {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::BASE;
};

template <COMPONENT_TYPE eComponentType>
struct ComponentIndexToType {
	using type = void;
};

template <>
struct ComponentIndexToType<COMPONENT_TYPE::BASE> {
	using type = IComponent;
};
