#include "VkResourceDeleter.h"
#include "DeviceVk.h"
#include "QueueVk.h"

#include <assert.h>
namespace rhi::vulkan
{
	VkResourceDeleter::VkResourceDeleter(Queue* queue) : mQueue(queue) {}

	VkResourceDeleter::~VkResourceDeleter()
	{
        assert(mBuffersToDelete.Empty());
        assert(mDescriptorPoolsToDelete.Empty());
        assert(mFencesToDelete.Empty());
        assert(mImagesToDelete.Empty());
        assert(mImageViewsToDelete.Empty());
        assert(mPipelinesToDelete.Empty());
        assert(mPipelineLayoutsToDelete.Empty());
        assert(mQueryPoolsToDelete.Empty());
        assert(mSamplersToDelete.Empty());
        assert(mSemaphoresToDelete.Empty());
        assert(mShaderModulesToDelete.Empty());
	}

    void VkResourceDeleter::Tick(uint64_t completedSerial)
    {
        Device* device = mQueue->GetDevice();
        VkDevice vkDeivce = device->GetHandle();
        VmaAllocator vmaAllocator = device->GetMemoryAllocator();

        for (BufferAllocation bufferAllocation : mBuffersToDelete.IterateUpTo(completedSerial))
        {
            vmaDestroyBuffer(vmaAllocator, bufferAllocation.buffer, bufferAllocation.allocation);
        }
        mBuffersToDelete.ClearUpTo(completedSerial);

        for (ImageAllocation imageAllocation : mImagesToDelete.IterateUpTo(completedSerial))
        {
            vmaDestroyImage(vmaAllocator, imageAllocation.image, imageAllocation.allocation);
        }
        mImagesToDelete.ClearUpTo(completedSerial);

        for (VkPipelineLayout layout : mPipelineLayoutsToDelete.IterateUpTo(completedSerial))
        {
            vkDestroyPipelineLayout(vkDeivce, layout, nullptr);
        }
        mPipelineLayoutsToDelete.ClearUpTo(completedSerial);

        for (VkPipeline pipeline : mPipelinesToDelete.IterateUpTo(completedSerial)) {
            vkDestroyPipeline(vkDeivce, pipeline, nullptr);
        }
        mPipelinesToDelete.ClearUpTo(completedSerial);

        for (VkImageView view : mImageViewsToDelete.IterateUpTo(completedSerial))
        {
            vkDestroyImageView(vkDeivce, view, nullptr);
        }
        mImageViewsToDelete.ClearUpTo(completedSerial);

        for (VkSampler sampler : mSamplersToDelete.IterateUpTo(completedSerial))
        {
            vkDestroySampler(vkDeivce, sampler, nullptr);
        }
        mSamplersToDelete.ClearUpTo(completedSerial);

        for (VkShaderModule module : mShaderModulesToDelete.IterateUpTo(completedSerial))
        {
            vkDestroyShaderModule(vkDeivce, module, nullptr);
        }
        mShaderModulesToDelete.ClearUpTo(completedSerial);

        for (VkDescriptorPool pool : mDescriptorPoolsToDelete.IterateUpTo(completedSerial))
        {
            vkDestroyDescriptorPool(vkDeivce, pool, nullptr);
        }
        mDescriptorPoolsToDelete.ClearUpTo(completedSerial);

        for (ConcurrentBufferAllocation* bufferAllocation : mConcurrentBuffersToDelete.IterateUpTo(completedSerial))
        {
            --bufferAllocation->refQueueCount;
            if (bufferAllocation->refQueueCount == 0)
            {
                vmaDestroyBuffer(vmaAllocator, bufferAllocation->buffer, bufferAllocation->allocation);
                delete bufferAllocation;
            }
        }
        mConcurrentBuffersToDelete.ClearUpTo(completedSerial);

        for (ConcurrentImageAllocation* imageAllocation : mConcurrentImagesToDelete.IterateUpTo(completedSerial))
        {
            --imageAllocation->refQueueCount;
            if (imageAllocation->refQueueCount == 0)
            {
                vmaDestroyImage(vmaAllocator, imageAllocation->image, imageAllocation->allocation);
                delete imageAllocation;
            }
        }
        mConcurrentImagesToDelete.ClearUpTo(completedSerial);
    }

    void VkResourceDeleter::DeleteWhenUnused(BufferAllocation bufferAllocation)
    {
        mBuffersToDelete.Push(mQueue->GetPendingSubmitSerial(), bufferAllocation);
    }

    void VkResourceDeleter::DeleteWhenUnused(VkDescriptorPool pool)
    {
        mDescriptorPoolsToDelete.Push(mQueue->GetPendingSubmitSerial(), pool);
    }

    void VkResourceDeleter::DeleteWhenUnused(ImageAllocation imageAllocation)
    {
        mImagesToDelete.Push(mQueue->GetPendingSubmitSerial(), imageAllocation);
    }

    void VkResourceDeleter::DeleteWhenUnused(VkImageView view)
    {
        mImageViewsToDelete.Push(mQueue->GetPendingSubmitSerial(), view);
    }

    void VkResourceDeleter::DeleteWhenUnused(VkPipeline pipeline)
    {
        mPipelinesToDelete.Push(mQueue->GetPendingSubmitSerial(), pipeline);
    }

    void VkResourceDeleter::DeleteWhenUnused(VkPipelineLayout layout)
    {
        mPipelineLayoutsToDelete.Push(mQueue->GetPendingSubmitSerial(), layout);
    }

    void VkResourceDeleter::DeleteWhenUnused(VkSampler sampler)
    {
        mSamplersToDelete.Push(mQueue->GetPendingSubmitSerial(), sampler);
    }

    void VkResourceDeleter::DeleteWhenUnused(VkShaderModule module)
    {
        mShaderModulesToDelete.Push(mQueue->GetPendingSubmitSerial(), module);
    }

    void VkResourceDeleter::DeleteWhenUnused(VkSemaphore semaphore)
    {
        mSemaphoresToDelete.Push(mQueue->GetPendingSubmitSerial(), semaphore);
    }

    void VkResourceDeleter::DeleteWhenUnused(ConcurrentBufferAllocation* buffer)
    {
        mConcurrentBuffersToDelete.Push(mQueue->GetPendingSubmitSerial(), buffer);
    }

    void VkResourceDeleter::DeleteWhenUnused(ConcurrentImageAllocation* image)
    {
        mConcurrentImagesToDelete.Push(mQueue->GetPendingSubmitSerial(), image);
    }
}