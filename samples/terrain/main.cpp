#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "frustum.h"

#include "terrain.h"

int main()
{
	Terrain terrain;
	terrain.create(1024, 720);
	terrain.init();
	terrain.run();
}