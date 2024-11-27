#pragma once

#include "rhi/rhi.h"
#include <vector>

struct Mesh
{
	rhi::IBuffer* vertexBuffer;
	rhi::IBuffer* indexBuffer;
};

inline Mesh createPlaneMesh(rhi::IRenderDevice* rd, int dimension)
{
	struct Vertex
	{
		float positions[3];
		uint16_t uv[2];
	};

	int stride = 1;
	int verticesPerLine = dimension / stride + 1;

	std::vector<Vertex> vertices(verticesPerLine * verticesPerLine);
	std::vector<uint32_t> indices(dimension * dimension * 6);

	uint32_t vertexCount = 0;
	for (int z = 0; z < verticesPerLine; ++z)
	{
		for (int x = 0; x < verticesPerLine; ++x)
		{
			auto& vertex = vertices[vertexCount++];
			vertex.positions[0] = static_cast<float>(x * stride);
			vertex.positions[1] = 0.0f;
			vertex.positions[2] = static_cast<float>(z * stride);
			vertex.uv[0] = static_cast<uint16_t>(x % 2);
			vertex.uv[1] = static_cast<uint16_t>(z % 2);
		}
	}
	int indexCount = 0;
	for (int z = 0; z < dimension; ++z)
	{
		for (int x = 0; x < dimension; ++x)
		{
			int baseIndex = z * verticesPerLine + x;
			indices[indexCount++] = baseIndex + 1;
			indices[indexCount++] = baseIndex + 1 + verticesPerLine;
			indices[indexCount++] = baseIndex + verticesPerLine;
			indices[indexCount++] = baseIndex + 1;
			indices[indexCount++] = baseIndex + verticesPerLine;
			indices[indexCount++] = baseIndex;
		}
	}
	Mesh mesh{};
	rhi::BufferDesc desc{};
	desc.size = vertices.size() * sizeof(Vertex);
	desc.access = rhi::BufferAccess::GpuOnly;
	desc.usage = rhi::BufferUsage::VertexBuffer;
	mesh.vertexBuffer = rd->createBuffer(desc, vertices.data(), desc.size);
	desc.size = indices.size() * sizeof(uint32_t);
	desc.usage = rhi::BufferUsage::IndexBuffer;
	mesh.indexBuffer = rd->createBuffer(desc, indices.data(), desc.size);
	return mesh;
}