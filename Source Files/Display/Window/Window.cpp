#include "Window.h"
#include "Utility\Logger.h"
#include "Raster Engine\RasterBackend.h"
#include "Multithreading\Scheduler.h"
#include "WindowManager.h"
#include "Input/InputManager.h"

/*
 *    Constructor.
 *    @param 		 	inWin		 	The in window.
 *    @param 		 	inTitle		 	The in title.
 *    @param 		 	x			 	An uint to process.
 *    @param 		 	y			 	An uint to process.
 *    @param 		 	iwidth		 	The iwidth.
 *    @param 		 	iheight		 	The iheight.
 *    @param [in,out]	monitorIn	 	If non-null, the monitor in.
 *    @param [in,out]	sharedContext	If non-null, context for the shared.
 */

Window::Window(WindowType inWin, const std::string& inTitle, uint x, uint y, uint iwidth, uint iheight, GLFWmonitor* monitorIn, GLFWwindow* sharedContext)
{
	windowType = inWin;
	xPos = x;
	yPos = y;
	width = iwidth;
	height = iheight;
	title = inTitle;
	windowHandle = nullptr;

	Scheduler::AddTask(LAUNCH_IMMEDIATE, FIBER_HIGH, true, [this, sharedContext, monitorIn ]() {

		RasterBackend::SetWindowHints();
		glfwWindowHint(GLFW_SAMPLES, GLFW_DONT_CARE);
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

		const GLFWvidmode* mode = glfwGetVideoMode(monitorIn);

		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		if (windowType == FULLSCREEN) {

			glfwWindowHint(GLFW_RESIZABLE, false);
			glfwWindowHint(GLFW_DECORATED, false);

			windowHandle = glfwCreateWindow(width, height, title.c_str(), monitorIn, sharedContext);

		}
		else if (windowType == WINDOWED) {

			glfwWindowHint(GLFW_RESIZABLE, true);

			windowHandle = glfwCreateWindow(width, height, title.c_str(), nullptr, sharedContext);

		}
		else if (windowType == BORDERLESS) {

			glfwWindowHint(GLFW_RESIZABLE, false);
			glfwWindowHint(GLFW_DECORATED, false);

			windowHandle = glfwCreateWindow(width, height, title.c_str(), nullptr, sharedContext);

		}
		else {
			glfwWindowHint(GLFW_RESIZABLE, false);
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

			windowHandle = glfwCreateWindow(width, height, title.c_str(), nullptr, sharedContext);

		}

	});

	Scheduler::Block();

	if (windowHandle == nullptr)
	{
		S_LOG_FATAL("Could not Create GLFW Window");
	}

	RasterBackend::BuildWindow(windowHandle);

	//the backend is the new user

	Window* thisWindow = this;

	Scheduler::AddTask(LAUNCH_IMMEDIATE, FIBER_HIGH, true, [this, thisWindow]() {
		glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		glfwSetWindowUserPointer(windowHandle, thisWindow);

		//all window related callbacks
		glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* w, int x, int y)
		{
			WindowManager::Resize(w, x, y);
		});

		glfwSetWindowPosCallback(windowHandle, [](GLFWwindow* w, int x, int y)
		{
			WindowManager::WindowPos(w, x, y);
		});

		glfwSetWindowRefreshCallback(windowHandle, [](GLFWwindow* w)
		{
			WindowManager::Refresh(w);
		});

		glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* w)
		{
			WindowManager::Close(w);
		});

		glfwShowWindow(windowHandle);

	});

	InputManager::AttachWindow(windowHandle);

	Scheduler::Block();

}

/* Destructor. */
Window::~Window()
{
	Scheduler::AddTask(LAUNCH_IMMEDIATE, FIBER_HIGH, true, [this]() {
		if (windowHandle) {
			glfwDestroyWindow(windowHandle);
		}
	});

	Scheduler::Block();
}

/* Draws this object. */
void Window::Draw()
{

	RasterBackend::PreRaster(windowHandle);
	layout->Draw();
	RasterBackend::PostRaster(windowHandle);

}