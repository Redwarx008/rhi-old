#include "app_base.h"
#include <iostream>


using namespace rhi;

AppBase::~AppBase()
{
	delete m_SwapChain;
	delete m_RenderDevice;
}

void AppBase::create(uint32_t width, uint32_t height)
{
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW\n";
		return;
	}
	m_WindowWidth = width;
	m_WindowHeight = height;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, "terrain", nullptr, nullptr);
	if (!m_Window)
	{
		std::cerr << "Failed to create a glfw window\n";
		glfwTerminate();
		return;
	}

	glfwSetWindowUserPointer(m_Window, this);

	RenderDeviceCreateInfo rdCI{};
	rdCI.enableValidationLayer = true;
	m_RenderDevice = createRenderDevice(rdCI);

	SwapChainCreateInfo spCI{};
	spCI.initialWidth = m_WindowWidth;
	spCI.initialHeight = m_WindowHeight;
	spCI.windowHandle = glfwGetWin32Window(m_Window);
	spCI.renderDevice = m_RenderDevice;
	spCI.enableVSync = true;
	m_SwapChain = createSwapChain(spCI);
}

void AppBase::run()
{
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();

		update();

		int w, h;
		glfwGetWindowSize(m_Window, &w, &h);

		m_SwapChain->beginFrame();
		draw();
		m_SwapChain->present();
	}
}