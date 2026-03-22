#pragma once
#include "pch.h"

namespace AIDLL
{
	class PathEdge
	{
	private:
		// 이 엣지가 연결하는 시작/끝 위치 (3D)
		Vector3 m_v3Source;
		Vector3 m_v3Destination;

	public:

		PathEdge(Vector3 Source, Vector3 Destination) : m_v3Source(Source), m_v3Destination(Destination) {}

		Vector3 Destination() const { return m_v3Destination; }
		void SetDestination(Vector3 NewDest) { m_v3Destination = NewDest; }

		Vector3 Source() const { return m_v3Source; }
		void SetSource(Vector3 NewSource) { m_v3Source = NewSource; }
	};
}
