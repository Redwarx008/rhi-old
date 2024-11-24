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
	std::vector<Vertex> vertices(dimension * dimension * 2);
	std::vector<uint32_t> indices(vertices.size() * 3);

	int stride = 1;
	int verticesPerLine = dimension / stride + 1;

	for (int z = 0; z < verticesPerLine; ++z)
	{
		for (int x = 0; x < verticesPerLine; ++x)
		{
			vertices.push_back({ {(float)x * stride, 0.0f, (float)z * stride}, {static_cast<uint16_t>(x % 2), static_cast<uint16_t>(z % 2)} });
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
	desc.size = vertices.size();
	desc.access = rhi::BufferAccess::GpuOnly;
	desc.usage = rhi::BufferUsage::VertexBuffer;
	mesh.vertexBuffer = rd->createBuffer(desc, vertices.data(), vertices.size());
	desc.size = indices.size();
	desc.usage = rhi::BufferUsage::IndexBuffer;
	mesh.indexBuffer = rd->createBuffer(desc, indices.data(), indices.size());
	return mesh;
}