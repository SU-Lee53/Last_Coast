#include "pch.h"
#include "NavMeshImpl.h"


namespace AIDLL
{
	NavMeshImpl::NavMeshImpl()
	{
	}

	NavMeshImpl::~NavMeshImpl()
	{
	}

	bool NavMeshImpl::LoadFromJson(const char* filepath)
	{
		std::ifstream file(filepath);
		if (!file.is_open())
		{
			return false;
		}

		nlohmann::json jNavMesh;
		file >> jNavMesh;
		file.close();

		// Config 로드
		if (jNavMesh.contains("Config"))
		{
			auto& jConfig = jNavMesh["Config"];
			m_Config.CellSize = jConfig["CellSize"].get<float>();
			m_Config.CellHeight = jConfig["CellHeight"].get<float>();
			m_Config.AgentRadius = jConfig["AgentRadius"].get<float>();
			m_Config.AgentHeight = jConfig["AgentHeight"].get<float>();
			m_Config.AgentMaxSlope = jConfig["AgentMaxSlope"].get<float>();
			m_Config.AgentMaxStepHeight = jConfig["AgentMaxStepHeight"].get<float>();
		}

		// Tiles 로드
		if (jNavMesh.contains("Tiles"))
		{
			for (auto& jTile : jNavMesh["Tiles"])
			{
				Internal::NavMeshTile tile;

				tile.X = jTile["X"].get<int>();
				tile.Y = jTile["Y"].get<int>();
				tile.Layer = jTile["Layer"].get<int>();

				// ✅ SimpleMath Vector3 사용
				auto& jBounds = jTile["Bounds"];
				tile.BoundsMin = Vector3(
					jBounds["MinX"].get<float>(),
					jBounds["MinY"].get<float>(),
					jBounds["MinZ"].get<float>()
				);
				tile.BoundsMax = Vector3(
					jBounds["MaxX"].get<float>(),
					jBounds["MaxY"].get<float>(),
					jBounds["MaxZ"].get<float>()
				);

				// Vertices
				for (auto& jVert : jTile["Vertices"])
				{
					Internal::NavMeshVertex vertex;
					vertex.Position = Vector3(
						jVert["X"].get<float>(),
						jVert["Y"].get<float>(),
						jVert["Z"].get<float>()
					);
					tile.Vertices.push_back(vertex);
				}

				// Polygons
				for (auto& jPoly : jTile["Polygons"])
				{
					Internal::NavMeshPolygon polygon;
					for (auto& jIndex : jPoly["Indices"])
					{
						polygon.Indices.push_back(jIndex.get<int>());
					}
					polygon.AreaType = jPoly["AreaType"].get<int>();
					polygon.Flags = jPoly["Flags"].get<int>();
					tile.Polygons.push_back(polygon);
				}

				m_Tiles.push_back(tile);
			}
		}

		return true;
	}

	bool NavMeshImpl::IsPointOnNavMesh(const Vector3& point) const
	{
		for (const auto& tile : m_Tiles)
		{
			// ✅ SimpleMath Vector3 사용
			if (point.x < tile.BoundsMin.x || point.x > tile.BoundsMax.x ||
				point.y < tile.BoundsMin.y || point.y > tile.BoundsMax.y ||
				point.z < tile.BoundsMin.z || point.z > tile.BoundsMax.z)
			{
				continue;
			}

			for (const auto& poly : tile.Polygons)
			{
				if (IsPointInPolygon(point, poly, tile))
				{
					return true;
				}
			}
		}

		return false;
	}

	Vector3 NavMeshImpl::GetNearestPointOnNavMesh(const Vector3& point) const
	{
		float minDistance = FLT_MAX;
		Vector3 nearestPoint = point;

		for (const auto& tile : m_Tiles)
		{
			for (const auto& poly : tile.Polygons)
			{
				for (int idx : poly.Indices)
				{
					if (idx < tile.Vertices.size())
					{
						Vector3 vertPos = tile.Vertices[idx].Position;

						// ✅ SimpleMath의 Distance 사용
						float distance = Vector3::Distance(point, vertPos);

						if (distance < minDistance)
						{
							minDistance = distance;
							nearestPoint = vertPos;
						}
					}
				}
			}
		}

		return nearestPoint;
	}

	bool NavMeshImpl::FindPath(const Vector3& start, const Vector3& end, std::vector<Vector3>& outPath)
	{
		outPath.clear();

		// 간단한 버전: 직선 경로
		outPath.push_back(start);
		outPath.push_back(end);

		// TODO: A* 구현

		return true;
	}

	bool NavMeshImpl::IsPointInPolygon(const Vector3& point, const Internal::NavMeshPolygon& poly, const Internal::NavMeshTile& tile) const
	{
		if (poly.Indices.size() < 3)
			return false;

		int crossings = 0;

		for (size_t i = 0; i < poly.Indices.size(); i++)
		{
			int idx1 = poly.Indices[i];
			int idx2 = poly.Indices[(i + 1) % poly.Indices.size()];

			if (idx1 >= tile.Vertices.size() || idx2 >= tile.Vertices.size())
				continue;

			Vector3 v1 = tile.Vertices[idx1].Position;
			Vector3 v2 = tile.Vertices[idx2].Position;

			// 2D 점-폴리곤 테스트 (XZ 평면)
			if (((v1.z <= point.z && point.z < v2.z) ||
				(v2.z <= point.z && point.z < v1.z)) &&
				(point.x < (v2.x - v1.x) * (point.z - v1.z) / (v2.z - v1.z) + v1.x))
			{
				crossings++;
			}
		}

		return (crossings % 2) == 1;
	}

	float NavMeshImpl::DistanceToPolygon(const Vector3& point, const Internal::NavMeshPolygon& poly, const Internal::NavMeshTile& tile) const
	{
		float minDist = FLT_MAX;

		for (int idx : poly.Indices)
		{
			if (idx < tile.Vertices.size())
			{
				Vector3 vertPos = tile.Vertices[idx].Position;
				// ✅ SimpleMath의 Distance 사용
				float dist = Vector3::Distance(point, vertPos);
				minDist = std::min(minDist, dist);
			}
		}

		return minDist;
	}
}
