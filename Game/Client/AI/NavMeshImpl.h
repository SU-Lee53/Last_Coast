#pragma once
#include "AI.h"



namespace AIDLL
{
	namespace Internal  // 내부 구현용
	{
		struct NavMeshVertex
		{
			Vector3 Position;
		};

		struct NavMeshPolygon
		{
			std::vector<int> Indices;
			int AreaType;
			int Flags;
		};

		struct NavMeshTile
		{
			int X, Y, Layer;
			Vector3 BoundsMin;
			Vector3 BoundsMax;
			std::vector<NavMeshVertex> Vertices;
			std::vector<NavMeshPolygon> Polygons;
		};
	}

	class NavMeshImpl : public INavMesh
	{
	public:
		NavMeshImpl();
		virtual ~NavMeshImpl();

		// INavMesh 구현
		virtual bool LoadFromJson(const char* filepath) override;
		virtual bool IsPointOnNavMesh(const Vector3& point) const override;
		virtual Vector3 GetNearestPointOnNavMesh(const Vector3& point) const override;
		virtual bool FindPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath) override;
		virtual NavMeshConfig GetConfig() const override { return m_Config; }
		virtual int GetTileCount() const override { return static_cast<int>(m_Tiles.size()); }

	private:
		NavMeshConfig m_Config;
		std::vector<Internal::NavMeshTile> m_Tiles;

		bool IsPointInPolygon(const Vector3& point, const Internal::NavMeshPolygon& poly, const Internal::NavMeshTile& tile) const;
		float DistanceToPolygon(const Vector3& point, const Internal::NavMeshPolygon& poly, const Internal::NavMeshTile& tile) const;
	};
}

