#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 worldSpacePos;

layout(binding = 0) uniform sampler2D heightmap;

layout(binding = 3, std430) uniform TerrainParams
{
	float heightScale;
	uint baseChunkSize;
	uvec2 heightmapSize;
};

layout (location = 0) out vec4 outFragColor;


vec3 calculateNormal(vec2 uv)
{
	vec2 texelSize = 1.0 / vec2(heightmapSize);
	float left = textureLod(heightmap, uv + vec2(-texelSize.x, 0), 0).r * heightScale;
	float right = textureLod(heightmap, uv + vec2(texelSize.x, 0), 0).r * heightScale;
	float up = textureLod(heightmap, uv + vec2(0, -texelSize.y), 0).r * heightScale;
	float down = textureLod(heightmap, uv + vec2(0, texelSize.y), 0).r * heightScale;
	return normalize(vec3(left - right, 2.0, up - down));
}


void main()
{
	vec3 lightPos = vec3(1000, 10000, 1000);
	vec3 lightDir = normalize(lightPos - worldSpacePos);
	vec3 normal = calculateNormal(uv);
	vec3 ambient = vec3(0.25);
	vec3 diffuse = vec3(max(dot(normal, lightDir), 0.0));
	vec3 color = vec3(0.5,0.5,0.5);
	//outFragColor = vec4((ambient + diffuse) * color, 1.0);
	outFragColor = vec4(normal.xzy, 1);
}