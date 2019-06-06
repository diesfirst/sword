#include "context.hpp"

//public

Context::Context()
{
	createContext();
}

Context::~Context()
{
	destroyContext();
}

void Context::deviceReport()
{
	printDeviceInfo();
	printDeviceMemoryHeapInfo();
	printDeviceMemoryTypeInfo();
	printDeviceQueueFamilyInfo();
//	printInstanceExtensionProperties();
	printDeviceExtensionProperties();
}

vk::Device Context::getDevice()
{
	return device;
}

//private

void Context::createContext()
{
	checkInstanceExtensionProperties();
	printInstanceExtensionProperties();
	createInstance();
	if (enableValidation) setupDebugMessenger();
	createPhysicalDevice();
	createDevice();
}

void Context::destroyContext()
{
	device.destroy();
	instance.destroy();
}

void Context::createInstance()
{
	std::vector<const char*> extensions;
	extensions.push_back("VK_EXT_debug_utils");
	extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	std::vector<const char*> layers;
	layers.push_back("VK_LAYER_LUNARG_standard_validation");

	vk::InstanceCreateInfo instanceInfo;
	instanceInfo.enabledExtensionCount = extensions.size();
	instanceInfo.ppEnabledExtensionNames = extensions.data();
	instanceInfo.enabledLayerCount = layers.size();
	instanceInfo.ppEnabledLayerNames = layers.data();

	instance = vk::createInstance(instanceInfo);
}

void Context::checkInstanceExtensionProperties()
{
	instanceExtensionProperties = vk::enumerateInstanceExtensionProperties();
}

void Context::createPhysicalDevice()
{
	physicalDevice = instance.enumeratePhysicalDevices()[0];
	physicalDeviceProperties = physicalDevice.getProperties();
	physicalDeviceMemoryProperties = physicalDevice.getMemoryProperties();
	physicalDeviceFeatures = physicalDevice.getFeatures();
	queueFamilies = physicalDevice.getQueueFamilyProperties();
	deviceExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
}

void Context::createDevice()
{
	float queuePriority = 1.0; //apparently necesary? 
	vk::DeviceQueueCreateInfo queueInfo;
	queueInfo.flags = vk::DeviceQueueCreateFlags();
	queueInfo.queueCount = 1;
	queueInfo.queueFamilyIndex = 0; //should be the one with graphics capabilities
	queueInfo.pQueuePriorities = &queuePriority;
	
	vk::DeviceCreateInfo deviceInfo;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;

	std::vector<const char*> extensions;
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	deviceInfo.enabledExtensionCount = extensions.size();
	deviceInfo.ppEnabledExtensionNames = extensions.data();

	device = physicalDevice.createDevice(deviceInfo);
}

void Context::setGraphicsQueue()
{
	graphicsQueue = device.getQueue(0, 0);
}

void Context::setPresentQueue()
{
	presentQueue = device.getQueue(0, 0);
}

void Context::printInstanceExtensionProperties()
{
	std::cout << "---Instance Extension Properties---" << std::endl;
	for (const auto n : instanceExtensionProperties) {
		std::cout << "Instance extension name: " << n.extensionName << std::endl;
	}
}

void Context::printDeviceMemoryHeapInfo()
{
	std::cout << "Memory Heaps: " << std::endl;
	int heapCount = physicalDeviceMemoryProperties.memoryHeapCount;
	for (int i = 0; i < heapCount; ++i) {
		const auto& heap = physicalDeviceMemoryProperties.memoryHeaps[i];
		std::cout << 
			"Heap: " << i << 
			"\tflags: " << vk::to_string(heap.flags) << 
			"\tSize: " << heap.size << std::endl;
	}
}

void Context::printDeviceMemoryTypeInfo()
{
	std::cout << "Memory types: " << std::endl;
	int typeCount = physicalDeviceMemoryProperties.memoryTypeCount;
	for (int i = 0; i < typeCount; ++i) {
		const auto& type = physicalDeviceMemoryProperties.memoryTypes[i];
		std::cout << 
		       "Type: " << i <<
		       " flags " << vk::to_string(type.propertyFlags) <<
		       " heap " << type.heapIndex << std::endl;
	}
}

void Context::printDeviceInfo()
{
	std::cout << "Device Name: " << physicalDeviceProperties.deviceName << std::endl;
}

void Context::printDeviceFeatures()
{

}


void Context::printDeviceQueueFamilyInfo()
{
	std::cout << "Queues: "	<< std::endl;
	for (int i = 0; i < queueFamilies.size(); ++i) {
		const auto& queueFamilyProperties = queueFamilies[i];
		std::cout << "Queue Family: " << i << std::endl;
		std::cout << "Queue Family Flags: " << 
			vk::to_string(queueFamilyProperties.queueFlags) << std::endl;
		std::cout << "QueueCount: " << queueFamilyProperties.queueCount << std::endl;
	}
}

void Context::printDeviceExtensionProperties()
{
	std::cout << "Device Extension Properties: " << std::endl;
	for (const auto exProp : deviceExtensionProperties) {
		std::cout << "Name: " << exProp.extensionName << std::endl;
	}
}

uint32_t Context::pickQueueFamilyIndex(vk::SurfaceKHR surface) const
{
	uint32_t index = 0;
	vk::QueueFamilyProperties familyProperties = queueFamilies[index];
	const vk::QueueFlags& desiredFlags = vk::QueueFlagBits::eGraphics;
	if (familyProperties.queueFlags & desiredFlags)
	{
		std::cout << 
			"QueueFamily index " <<
			index <<
			" is graphics suitable" << 
			std::endl;
	}
	if (physicalDevice.getSurfaceSupportKHR(index, surface))
	{
		std::cout << 
			"QueueFamily index " <<
			index <<
			" is presentation suitable" << 
			std::endl;
	}
	return index;
}

void Context::setDeviceExtensions(vk::DeviceCreateInfo& createInfo)
{
	const uint8_t extCount = 1;
	const char* names[extCount];
	names[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
	createInfo.enabledExtensionCount = extCount;
	createInfo.ppEnabledExtensionNames = names;
}

void Context::setupDebugMessenger()
{
	vk::DebugUtilsMessengerCreateInfoEXT dbCreateInfo;
	dbCreateInfo.messageSeverity =
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose;
	dbCreateInfo.messageType =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
	dbCreateInfo.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugCallback;
	std::cout << "going for it" << std::endl;
	dispatcher.init(instance);
	auto messenger = instance.createDebugUtilsMessengerEXT(
			dbCreateInfo, 
			nullptr,
			dispatcher);
	std::cout << "Made it" << std::endl;
}

