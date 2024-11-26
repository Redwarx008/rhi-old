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
    enum class Key
    {
        Esc = GLFW_KEY_ESCAPE,
        Space = GLFW_KEY_SPACE,
        Tab = GLFW_KEY_TAB,

        W = GLFW_KEY_W,
        A = GLFW_KEY_A,
        S = GLFW_KEY_S,
        D = GLFW_KEY_D,

        // arrows
        Left = GLFW_KEY_LEFT,
        Right = GLFW_KEY_RIGHT,
        Up = GLFW_KEY_UP,
        Down = GLFW_KEY_DOWN,

        // numpad arrows
        NP_Left = GLFW_KEY_KP_4,
        NP_Right = GLFW_KEY_KP_6,
        NP_Up = GLFW_KEY_KP_8,
        NP_Down = GLFW_KEY_KP_2,

        // mouse buttons
        MB_Left = GLFW_MOUSE_BUTTON_LEFT,
        MB_Right = GLFW_MOUSE_BUTTON_RIGHT,
        MB_Middle = GLFW_MOUSE_BUTTON_MIDDLE,
    };
    enum class KeyState
    {
        Release = GLFW_RELEASE,
        Press = GLFW_PRESS,
        Repeat = GLFW_REPEAT,
    };
	virtual void update() = 0;
	virtual void draw() = 0;

	rhi::IRenderDevice* m_RenderDevice;
	rhi::ISwapChain* m_SwapChain;
protected:
    virtual void KeyEvent(Key key, KeyState state) = 0;

    virtual void MouseEvent(float posX, float posY) = 0;

	GLFWwindow* m_Window;
	uint32_t m_WindowWidth;
	uint32_t m_WindowHeight;
private:
    static void GLFW_KeyCallback(GLFWwindow* wnd, int key, int, int state, int);
    static void GLFW_MouseButtonCallback(GLFWwindow* wnd, int button, int state, int);
    static void GLFW_CursorPosCallback(GLFWwindow* wnd, double xpos, double ypos);
};