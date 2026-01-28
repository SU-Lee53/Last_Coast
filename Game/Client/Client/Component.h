#pragma once

class IGameObject;

enum class COMPONENT_TYPE : uint8 {
	TRANSFORM = 0,
	MESH_RENDERER,
	SKELETON,
	ANIMATION_CONTROLLER,
	COLLIDER,

	COUNT,

	BASE = std::numeric_limits<uint8>::max()
};

interface IComponent abstract : std::enable_shared_from_this<IComponent> {
public:
	IComponent(std::shared_ptr<IGameObject> pOwner) : m_wpOwner {pOwner} {}

	virtual void Initialize () = 0;
	virtual void Update() = 0;

	std::shared_ptr<IGameObject> GetOwner() {
		return m_wpOwner.lock();
	}

	void SetOwner(std::shared_ptr<IGameObject> pOwner) {
		m_wpOwner = pOwner;
	}

	bool IsOwnerExpired() {
		return m_wpOwner.expired();
	}

	virtual std::shared_ptr<IComponent> Copy(std::shared_ptr<IGameObject> pNewOwner) const = 0;

protected:
	std::weak_ptr<IGameObject> m_wpOwner;
	bool m_bInitialized = false;

};

template<typename C>
concept ComponentType = std::derived_from<C, IComponent>;

template <ComponentType T>
struct ComponentIndex {
	constexpr static COMPONENT_TYPE componentType = COMPONENT_TYPE::BASE;
	constexpr static std::underlying_type_t<COMPONENT_TYPE> index = std::to_underlying(COMPONENT_TYPE::BASE);
};

