#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <libshaderc/shaderc.hpp>
#include <set>
#include <vulkan/vulkan2.hpp>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

int main() {

    uint32_t width = 640;
    uint32_t height = 480;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(width, height, "Vulkan 101", nullptr, nullptr);

    glfwSetKeyCallback(window, key_callback);

    vk::ApplicationInfo appInfo("Hello Triangle", VK_MAKE_VERSION(1, 0, 0), "No Engine",
                                VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_0);

    auto glfwExtensionCount = 0u;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> glfwExtensionsVector(glfwExtensions, glfwExtensions + glfwExtensionCount);
    glfwExtensionsVector.push_back("VK_EXT_debug_utils");
    auto layers = std::vector<const char*>{ "VK_LAYER_LUNARG_standard_validation" };

    auto instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo{ vk::InstanceCreateFlags(), &appInfo, static_cast<uint32_t>(layers.size()),
                                layers.data(), static_cast<uint32_t>(glfwExtensionsVector.size()),
                                glfwExtensionsVector.data() });

    auto messenger = instance->createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT{ vk::DebugUtilsMessengerCreateFlagsEXT(),
                                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                    vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                    vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
                                                debugCallback },
        nullptr, vk::DispatchLoaderDynamic{ *instance });

    VkSurfaceKHR surfaceTmp;
    VkResult err = glfwCreateWindowSurface(*instance, window, nullptr, &surfaceTmp);

    vk::UniqueSurfaceKHR surface(surfaceTmp, *instance);

    auto physicalDevices = instance->enumeratePhysicalDevices();

    auto physicalDevice = physicalDevices[std::distance(
        physicalDevices.begin(), std::find_if(physicalDevices.begin(), physicalDevices.end(), [](const vk::PhysicalDevice& physicalDevice) {
            return strstr(physicalDevice.getProperties().deviceName, "Intel");
        }))];

    auto queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    size_t graphicsQueueFamilyIndex =
        std::distance(queueFamilyProperties.begin(),
                      std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(),
                                   [](vk::QueueFamilyProperties const& qfp) {
                                       return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
                                   }));

    size_t presentQueueFamilyIndex = [&]() {
        for (size_t i = 0; i < queueFamilyProperties.size(); i++) {
            if (physicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), surface.get())) {
                return i;
            }
        }
        return 0ul;
    }();

    std::set<size_t> uniqueQueueFamilyIndices = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

    std::vector<uint32_t> FamilyIndices = { uniqueQueueFamilyIndices.begin(),
                                            uniqueQueueFamilyIndices.end() };

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;

    float queuePriority = 0.0f;
    for (int queueFamilyIndex : uniqueQueueFamilyIndices) {
        queueCreateInfos.push_back(vk::DeviceQueueCreateInfo{ vk::DeviceQueueCreateFlags(),
                                                                 static_cast<uint32_t>(queueFamilyIndex),
                                                                 1, &queuePriority });
    }

    const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    vk::UniqueDevice device = physicalDevice.createDeviceUnique(
        vk::DeviceCreateInfo(vk::DeviceCreateFlags(), queueCreateInfos.size(), queueCreateInfos.data(),
                             0, nullptr, deviceExtensions.size(), deviceExtensions.data()));

    uint32_t imageCount = 2;

    auto sharingModeTuple = [&]() {
        // using SharingModeTuple = std::tuple<vk::SharingMode, uint32_t, uint32_t*>;
        if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
            return std::make_tuple(vk::SharingMode::eConcurrent, 2u, FamilyIndices.data());
        } else {
            return std::make_tuple(vk::SharingMode::eExclusive, 0u, static_cast<uint32_t*>(nullptr));
        }
    }();
    //needed for validation warnings
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);

    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
        vk::SwapchainCreateFlagsKHR(), surface.get(), imageCount, vk::Format::eB8G8R8A8Unorm,
        vk::ColorSpaceKHR::eSrgbNonlinear, VkExtent2D{ width, height }, 1, vk::ImageUsageFlagBits::eColorAttachment,
        std::get<vk::SharingMode>(sharingModeTuple), std::get<uint32_t>(sharingModeTuple),
        std::get<uint32_t*>(sharingModeTuple), vk::SurfaceTransformFlagBitsKHR::eIdentity,
        vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::PresentModeKHR::eFifo, true, nullptr);

    auto swapChain = device->createSwapchainKHRUnique(swapChainCreateInfo);

    const char kShaderSource[] = "#version 310 es\n"
                                 "void main() { int x = 5; }\n";
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(kShaderSource, shaderc_glsl_vertex_shader, "vertex shader", options);
    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << module.GetErrorMessage();
    }

    vk::Queue deviceQueue = device->getQueue(graphicsQueueFamilyIndex, 0);
    vk::Queue presentQueue = device->getQueue(graphicsQueueFamilyIndex, 0);

    while (!glfwWindowShouldClose(window)) {
        // Keep running

        glfwPollEvents();
    }
}