#pragma once

#include "rhi/RHIStruct.h"

#include <bitset>

namespace rhi
{
	
	class FeatureSet
	{
	public:
		void EnableFeature(FeatureName feature)
		{
			mSet.set(static_cast<uint32_t>(feature));
		}
		bool IsEnabled(FeatureName feature)
		{
			return mSet[static_cast<uint32_t>(feature)];
		}
	private:
		static const uint32_t cFeatureCount = 14;
		std::bitset<cFeatureCount> mSet;
	};
}