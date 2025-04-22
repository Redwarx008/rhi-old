#pragma once

#include <vulkan/vulkan.h>
#include "ResourceToDelete.h"
#include "../common/SerialQueue.hpp"

namespace rhi::vulkan
{
	class Queue;

	class VkResourceDeleter
	{
	public:
		explicit VkResourceDeleter(Queue* queue);
		~VkResourceDeleter();

        void Tick(uint64_t completedSerial);

        void DeleteWhenUnused(BufferAllocation buffer);
        void DeleteWhenUnused(ImageAllocation image);
        void DeleteWhenUnused(VkImageView view);
        void DeleteWhenUnused(VkDescriptorPool pool);
        void DeleteWhenUnused(VkPipelineLayout layout);
        void DeleteWhenUnused(VkPipeline pipeline);
        //void DeleteWhenUnused(VkQueryPool querypool);
        void DeleteWhenUnused(VkSampler sampler);
        void DeleteWhenUnused(VkSemaphore semaphore);
        void DeleteWhenUnused(VkShaderModule module);
        void DeleteWhenUnused(ConcurrentBufferAllocation* buffer);
        void DeleteWhenUnused(ConcurrentImageAllocation* image);

    private:
        Queue* mQueue;
        SerialQueue<uint64_t, BufferAllocation> mBuffersToDelete;
        SerialQueue<uint64_t, VkDescriptorPool> mDescriptorPoolsToDelete;
        SerialQueue<uint64_t, ImageAllocation> mImagesToDelete;
        SerialQueue<uint64_t, VkFence> mFencesToDelete;
        SerialQueue<uint64_t, VkImageView> mImageViewsToDelete;
        SerialQueue<uint64_t, VkPipeline> mPipelinesToDelete;
        SerialQueue<uint64_t, VkPipelineLayout> mPipelineLayoutsToDelete;
        SerialQueue<uint64_t, VkQueryPool> mQueryPoolsToDelete;
        SerialQueue<uint64_t, VkSampler> mSamplersToDelete;
        SerialQueue<uint64_t, VkSemaphore> mSemaphoresToDelete;
        SerialQueue<uint64_t, VkShaderModule> mShaderModulesToDelete;

        SerialQueue<uint64_t, ConcurrentBufferAllocation*> mConcurrentBuffersToDelete;
        SerialQueue<uint64_t, ConcurrentImageAllocation*> mConcurrentImagesToDelete;
	};
}