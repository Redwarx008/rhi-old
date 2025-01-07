#include "BufferVk.h"

namespace rhi::vulkan
{
	VkBufferUsageFlags BufferUsageConvert(BufferUsage usage)
	{
		VkBufferUsageFlags flags = 0;

	}
	Ref<Buffer> Buffer::Create(Device* device, const BufferDesc& desc)
	{
		Ref<Buffer> buffer = AcquireRef(new Buffer(device, desc));

	}

	Buffer::Buffer(Device* device, const BufferDesc& desc) :
		mSize(desc.size),
		mUsage(desc.usage),
		mDevice(device)
	{

	}

	bool Buffer::Initialize()
	{

	}

	void Buffer::MapAsync(MapMode mode, BufferMapCallback callback, void* userData)
	{

	}

	BufferUsage Buffer::GetUsage() const
	{
		return mUsage;
	}

	uint64_t Buffer::GetSize() const
	{
		return mSize;
	}


}