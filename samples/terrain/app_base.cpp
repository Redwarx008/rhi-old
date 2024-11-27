#include "app_base.h"
#include <iostream>
#include <chrono>


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
	glfwSetKeyCallback(m_Window, &GLFW_KeyCallback);
	glfwSetMouseButtonCallback(m_Window, &GLFW_MouseButtonCallback);
	glfwSetCursorPosCallback(m_Window, &GLFW_CursorPosCallback);

	RenderDeviceCreateInfo rdCI{};
	rdCI.enableDebugRuntime = true;
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
	auto tPrevious = std::chrono::high_resolution_clock::now();
	while (!glfwWindowShouldClose(m_Window))
	{
		glfwPollEvents();

		auto tCurrent = std::chrono::high_resolution_clock::now();
		auto tDiff = std::chrono::duration<double, std::milli>(tCurrent - tPrevious).count();
		tPrevious = tCurrent;
		float dt = (float)tDiff / 1000.0f;

		update(dt);

		int w , h;
		glfwGetWindowSize(m_Window, &w, &h);
		if (w > 0 && h > 0)
		{
			m_SwapChain->beginFrame();
			draw();
			m_SwapChain->present();
		}
	}
}

void AppBase::GLFW_KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	auto* pSelf = static_cast<AppBase*>(glfwGetWindowUserPointer(window));
	pSelf->KeyEvent(static_cast<Key>(key), static_cast<KeyState>(action));
}

void AppBase::GLFW_MouseButtonCallback(GLFWwindow* wnd, int button, int state, int)
{
	auto* pSelf = static_cast<AppBase*>(glfwGetWindowUserPointer(wnd));
	pSelf->KeyEvent(static_cast<Key>(button), static_cast<KeyState>(state));
}

void AppBase::GLFW_CursorPosCallback(GLFWwindow* wnd, double xpos, double ypos)
{
	float xscale = 1;
	float yscale = 1;
	glfwGetWindowContentScale(wnd, &xscale, &yscale);
	auto* pSelf = static_cast<AppBase*>(glfwGetWindowUserPointer(wnd));
	pSelf->MouseEvent(static_cast<float>(xpos * xscale), static_cast<float>(ypos * yscale));
}