#version 450
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uvec2 inUV;

layout(binding = 0) uniform sampler2D heightmap;

layout(binding = 1, std430) readonly buffer SelectedNodeList
{
	int count;
	vec4 data[]; //[nodeX, nodeY, lodLevel, morphValue]
}selectedNodeList;

layout(binding = 2, std430) uniform SceneData
{
	mat4 projectionMatrix;
	mat4 modelMatrix;
	mat4 viewMatrix;
	vec4 cameraPos;
};

layout(binding = 3, std430) uniform TerrainParams
{
	float heightScale;
	uint baseChunkSize;
	uvec2 heightmapSize;
};

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 worldSpacePos;

float GetOriginHeight(vec2 worldSpacePosXZ, uvec2 lodDection)
{
	float h1 = textureLod(heightmap, (worldSpacePosXZ - vec2(lodDection)) / vec2(heightmapSize), 0).r * heightScale;
	float h2 = textureLod(heightmap, (worldSpacePosXZ + vec2(lodDection)) / vec2(heightmapSize), 0).r * heightScale;
	return (h1 + h2) * 0.5;
}

void main()
{
	uvec2 lodDirection = inUV;
	uint lodLevel = uint(selectedNodeList.data[gl_InstanceIndex].z);
	uvec2 scaledLodDir = lodDirection << lodLevel;
	float chunkScale = float(1 << lodLevel);
	float chunkSize = baseChunkSize * chunkScale;
	vec3 positionInModel = inPosition;
	positionInModel.xz *= chunkScale;
	vec2 nodeXY = selectedNodeList.data[gl_InstanceIndex].xy;
	vec3 worldSpacePos =  positionInModel + vec3(nodeXY.x, 0.0, nodeXY.y) * chunkSize;
	//Remove the extra vertices
	worldSpacePos.xz = min(worldSpacePos.xz, vec2(heightmapSize) - vec2(1.0, 1.0));
	uv = worldSpacePos.xz / vec2(heightmapSize);

	float morphValue = selectedNodeList.data[gl_InstanceIndex].w;
	float originH = GetOriginHeight(worldSpacePos.xz, scaledLodDir);
	float height = textureLod(heightmap, uv, 0).r * heightScale;
	height = mix(originH, height, morphValue);
	worldSpacePos.y = height;
	gl_Position = projectionMatrix * viewMatrix * vec4(worldSpacePos, 1);
}