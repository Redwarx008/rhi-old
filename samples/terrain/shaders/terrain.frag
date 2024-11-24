#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec2 uv;

layout(binding = 0) uniform sampler2D heightmap;

layout(binding = 3, std430) uniform TerrainParams
{
	float heightScale;
	uint baseChunkSize;
	uvec2 heightmapSize;
};

layout (location = 0) out vec4 outFragColor;


vec3 CalculateNormal(vec2 uv)
{
	vec2 texelSize = 1.0 / vec2(heightmapSize);
	float left = textureLod(heightmap, uv + vec2(-texelSize.x, 0), 0).r;
	float right = textureLod(heightmap, uv + vec2(texelSize.x, 0), 0).r;
	float up = textureLod(heightmap, uv + vec2(0, -texelSize.y), 0).r;
	float down = textureLod(heightmap, uv + vec2(0, texelSize.y), 0).r;
	return normalize(vec3(left - right, 2.0, up - down));
}


void main()
{
	outFragColor = texture(heightmap, uv);
}