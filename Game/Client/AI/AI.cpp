#include "pch.h"
#include "AI.h"
#include "AIManagerImpl.h"

namespace AIDLL
{
	std::shared_ptr<IAIManager> CreateAIManager()
	{
		return std::make_shared<AIManagerImpl>();
	}
}
