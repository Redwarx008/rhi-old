#include <rhi/rhi.h>

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>

class AppBase
{
public:
	void create(uint32_t width, uint32_t height);
	virtual void init() = 0;
	void run();
	virtual ~AppBase();
protected:
	virtual void update() = 0;
	virtual void draw() = 0;

	rhi::IRenderDevice* m_RenderDevice;
	rhi::ISwapChain* m_SwapChain;
private:
	GLFWwindow* m_Window;
	uint32_t m_WindowWidth;
	uint32_t m_WindowHeight;
};