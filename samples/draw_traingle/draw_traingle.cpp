#include <memory>
#include <iostream>
#include <fstream>
#include <vector>

#include <rhi/rhi.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1
#include <GLFW/glfw3native.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "camera.hpp"

using namespace rhi;


static void messageCallback(MessageSeverity severity, const char* msg)
{
	std::cerr << msg;
}

static std::vector<uint32_t> loadShaderData(const char* filePath)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	std::vector<uint32_t> buffer;
	if (!file.is_open()) {
		return buffer;
	}

	size_t fileSize = (size_t)file.tellg();
	buffer.resize(fileSize / sizeof(uint32_t));
	//spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file

	//put file cursor at beginning
	file.seekg(0);

	//load the entire file into the buffer
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	//now that the file is loaded into the buffer, we can close it
	file.close();

	return buffer;
}

struct Vertex {
	float position[3];
	float color[3];
};

const std::vector<Vertex> vertices{
	{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
	{ { -1.0f,  1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
	{ {  0.0f, -1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

std::vector<uint32_t> indices{ 0, 1, 2 };

struct ShaderData {
	glm::mat4 projectionMatrix;
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
};

Camera camera;



class App
{
public:
	void init()
	{
		if (!glfwInit())
		{
			std::cerr << "Failed to initialize GLFW\n";
			return;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		m_Window = glfwCreateWindow(m_windowWidth, m_windowHeight, "draw_traingle", nullptr, nullptr);
		if (!m_Window)
		{
			std::cerr << "Failed to create a glfw window\n";
			glfwTerminate();
			return;
		}

		glfwSetWindowUserPointer(m_Window, this);


		RenderDeviceCreateInfo rdCI{};
		rdCI.enableValidationLayer = true;
		rdCI.enableGeometryShader = true;
		rdCI.enableTessellationShader = true;
		
		m_RenderDevice = std::unique_ptr<IRenderDevice>(createRenderDevice(rdCI));

		SwapChainCreateInfo spCI{};
		spCI.initialWidth = m_windowWidth;
		spCI.initialHeight = m_windowHeight;
		spCI.windowHandle = glfwGetWin32Window(m_Window);
		spCI.renderDevice = m_RenderDevice.get();
		spCI.enableVSync = true;
		m_SwapChain = std::unique_ptr<ISwapChain>(createSwapChain(spCI));
		// create shader 
		ShaderCreateInfo shaderCI{};
		shaderCI.type = ShaderType::Vertex;
		shaderCI.entry = "main";
		std::vector<uint32_t> buffer = loadShaderData("triangle.vert.spv");
		auto vertexShader = m_RenderDevice->createShader(shaderCI, buffer.data(), buffer.size() * sizeof(uint32_t));
		shaderCI.type = ShaderType::Fragment;
		buffer = loadShaderData("triangle.frag.spv");
		auto fragmentShader = m_RenderDevice->createShader(shaderCI, buffer.data(), buffer.size() * sizeof(uint32_t));

		// create vertex buffer and index buffer
		BufferDesc bufferDesc{};
		bufferDesc.access = BufferAccess::GpuOnly;
		bufferDesc.usage = BufferUsage::VertexBuffer;
		bufferDesc.size = static_cast<uint32_t>(vertices.size()) * sizeof(Vertex);
		m_VertexBuffer = m_RenderDevice->createBuffer(bufferDesc, vertices.data(), bufferDesc.size);

		bufferDesc.usage = BufferUsage::IndexBuffer;
		bufferDesc.size = indices.size() * sizeof(uint32_t);
		m_IndexBuffer = m_RenderDevice->createBuffer(bufferDesc, indices.data(), bufferDesc.size);

		bufferDesc.usage = BufferUsage::UniformBuffer;
		bufferDesc.size = sizeof(ShaderData);
		m_UniformBuffer = m_RenderDevice->createBuffer(bufferDesc);
		// These match the following shader layout (see triangle.vert):
		//	layout (location = 0) in vec3 inPos;
		//	layout (location = 1) in vec3 inColor;
		VertexInputAttribute vertexInputs[] =
		{
			{0, 0, Format::RGB32_FLOAT}, 
			{0, 1, Format::RGB32_FLOAT} 
		};

		// create resouce set and layout
		ResourceSetLayoutItem layoutItems[] = { ResourceSetLayoutItem::UniformBuffer(ShaderType::Vertex, 0) };

		m_ResourceSetLayout = m_RenderDevice->createResourceSetLayout(layoutItems, 1);
		m_ResourceSet = m_RenderDevice->createResourceSet(*m_ResourceSetLayout);

		ResourceSetItem items[] = { ResourceSetItem::UniformBuffer(m_UniformBuffer, 0) };

		m_RenderDevice->writeResourceSet(*m_ResourceSet, items, 1);

		// create pipeline
		GraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.primType = PrimitiveType::TriangleList;
		pipelineCI.vertexInputAttributes = vertexInputs;
		pipelineCI.vertexInputAttributeCount = 2;
		pipelineCI.vertexShader = vertexShader;
		pipelineCI.fragmentShader = fragmentShader;
		pipelineCI.resourceSetLayouts = &m_ResourceSetLayout;
		pipelineCI.resourceSetLayoutCount = 1;
		pipelineCI.renderTargetFormatCount = 1;
		pipelineCI.renderTargetFormats[0] = m_SwapChain->getRenderTargetFormat();
		pipelineCI.depthStencilFormat = m_SwapChain->getDepthStencilFormat();
		pipelineCI.depthStencilState.depthTestEnable = true;
		pipelineCI.rasterState.cullMode = CullMode::None;

		m_Pipeline = m_RenderDevice->createGraphicsPipeline(pipelineCI);

		delete vertexShader;
		delete fragmentShader;

		m_CmdList = m_RenderDevice->createCommandList();

		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -2.5f));
		camera.setRotation(glm::vec3(0.0f));
		camera.setPerspective(60.0f, (float)m_windowWidth / (float)m_windowHeight, 1.0f, 256.0f);

		m_GraphicState.pipeline = m_Pipeline;
		m_GraphicState.resourceSetCount = 1;
		m_GraphicState.resourceSets[0] = m_ResourceSet;
		m_GraphicState.renderTargetCount = 1;
		m_GraphicState.indexBuffer = IndexBufferBinding().setBuffer(m_IndexBuffer).setFormat(Format::R32_UINT).setOffset(0);
		m_GraphicState.vertexBufferCount = 1;
		m_GraphicState.vertexBuffers[0] = VertexBufferBinding().setBuffer(m_VertexBuffer).setSlot(0).setOffset(0);
		m_GraphicState.viewportCount = 1;
		m_GraphicState.viewports[0] = { (float)m_windowWidth, (float)m_windowHeight };
		m_GraphicState.clearRenderTarget = true;
	}

	void run()
	{
		while (!glfwWindowShouldClose(m_Window))
		{
			glfwPollEvents();

			m_SwapChain->beginFrame();
			// draw 
			render();
			m_SwapChain->present();
		}
	}

	void render()
	{
		// Update the uniform buffer for the next frame
		ShaderData shaderData{};
		shaderData.projectionMatrix = camera.matrices.perspective;
		shaderData.viewMatrix = camera.matrices.view;
		shaderData.modelMatrix = glm::mat4(1.0f);

		auto renderTarget = m_SwapChain->getCurrentRenderTargetTexture();
		auto depthStencil = m_SwapChain->getDepthStencilTexture();

		m_GraphicState.renderTargetTextures[0] = renderTarget;
		m_GraphicState.depthStencilTexture = depthStencil;
		// If no scissor is provided, it will be set to the corresponding viewport size.
		Rect scissor{ (int)m_windowWidth / 4 * 3, (int)m_windowHeight / 4 * 3 };

		m_CmdList->open();
		m_CmdList->updateBuffer(*m_UniformBuffer, &shaderData, sizeof(ShaderData), 0);
		m_CmdList->setGraphicsState(m_GraphicState);
		m_CmdList->setScissors(&scissor, 1);
		m_CmdList->drawIndexed(indices.size(), 1, 0, 0, 0);
		m_CmdList->close();

		m_RenderDevice->executeCommandLists(&m_CmdList, 1);
	}

	void cleanUp()
	{
		if (m_RenderDevice)
		{
			m_RenderDevice->waitIdle();
		}
		delete m_IndexBuffer;
		m_IndexBuffer = nullptr;
		delete m_VertexBuffer;
		m_VertexBuffer = nullptr;
		delete m_UniformBuffer;
		m_UniformBuffer = nullptr;
		delete m_Pipeline;
		m_Pipeline = nullptr;
		delete m_ResourceSetLayout;
		m_ResourceSetLayout = nullptr;
		delete m_ResourceSet;
		m_ResourceSet = nullptr;
		m_SwapChain = nullptr;
		m_RenderDevice = nullptr;
		glfwTerminate();
	}
private:
	IBuffer* m_UniformBuffer;
	IBuffer* m_VertexBuffer;
	IBuffer* m_IndexBuffer;
	IGraphicsPipeline* m_Pipeline;
	ICommandList* m_CmdList;
	IResourceSetLayout* m_ResourceSetLayout;
	IResourceSet* m_ResourceSet;

	uint32_t m_windowWidth = 1024;
	uint32_t m_windowHeight = 768;
	GLFWwindow* m_Window;

	GraphicsState m_GraphicState{};

	std::unique_ptr<ISwapChain> m_SwapChain;
	std::unique_ptr<IRenderDevice> m_RenderDevice;
};


int main()
{
	App app;
	app.init();
	app.run();
	app.cleanUp();
}