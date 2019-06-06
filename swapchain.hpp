#include "context.hpp"
#include "window.hpp"

class Swapchain
{
public:
	Swapchain (const Context& context);
	virtual ~Swapchain();

	void checkPresentModes();

private:
	const Context& context;
	vk::SurfaceKHR surface;
	Window window;
	uint32_t queueFamilyIndex;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::Format colorFormat = vk::Format::eB8G8R8A8Snorm;
	vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
	vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
	vk::Extent2D swapchainExtent;
	
	void createSurface();

	void setColorFormat();
	
	void getSurfaceCapabilities();
};
