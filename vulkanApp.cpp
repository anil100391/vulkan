
#include <set>
#include <array>
#include <iostream>
#include <stdexcept>
#include <optional>
#include <algorithm>
#include <fstream>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>

#include <cstring> // for memcpy

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "vulkanApp.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static const int MAX_FRAMES_IN_FLIGHT = 2;

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };

#ifdef NDEBUG
static const bool enableValidationLayers = false;
#else
static const bool enableValidationLayers = true;
#endif

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof( Vertex, texCoord );

        return attributeDescriptions;
    }
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static const std::vector<Vertex> vertices = { {{-2.0f, -1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
                                              {{ 1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                                              {{ 1.0f,  1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
                                              {{-2.0f,  1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}
                                            };

static const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct VulkanApp::QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct VulkanApp::SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static std::vector<char> readFile( const std::string &filename )
{
    std::ifstream file( filename, std::ios::ate | std::ios::binary );

    if ( !file.is_open() )
    {
        throw std::runtime_error( "failed to open file!" );
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer( fileSize );
    file.seekg( 0 );
    file.read( buffer.data(), fileSize );
    file.close();
    return buffer;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                     const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                     void *pUserData )
{
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );
    auto wp = GetWindowParams();
    _window = glfwCreateWindow(wp.width, wp.height, wp.title.c_str(), nullptr, nullptr);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice(_physicalDevice);
    createSwapChain(_physicalDevice);
    createImageViews();
    createRenderPass();
    createDiscriptorSetLayout();
    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool(_physicalDevice);
    createTextureImage("textures/texture.jpg");
    createTextureImageView();
    createTextureSampler();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
static VkResult CreateDebugUtilsMessengerEXT( VkInstance instance,
                                              const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                              const VkAllocationCallbacks *pAllocator,
                                              VkDebugUtilsMessengerEXT *pDebugMessenger )
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void DestroyDebugUtilsMessengerEXT( VkInstance instance,
                                    VkDebugUtilsMessengerEXT debugMessenger,
                                    const VkAllocationCallbacks *pAllocator )
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( instance, debugMessenger, pAllocator );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT &createInfo )
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::setupDebugMessenger()
{
    if ( !enableValidationLayers )
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo( createInfo );

    if ( CreateDebugUtilsMessengerEXT( _instance, &createInfo, nullptr, &_debugMessenger ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to set up debug messenger!" );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createSurface()
{
    if (glfwCreateWindowSurface(_instance, _window, nullptr, &_surface) != VK_SUCCESS)
        throw std::runtime_error("failed to create window surface!");
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::pickPhysicalDevice()
{
    uint32_t deviceCnt = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCnt, nullptr);
    if (deviceCnt == 0)
        throw std::runtime_error("failed to find GPUs with Vulkan support!");

    std::vector<VkPhysicalDevice> devices(deviceCnt);
    vkEnumeratePhysicalDevices(_instance, &deviceCnt, devices.data());

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    for (const auto &device : devices)
    {
        if (isDeviceSuitable(device))
        {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    _physicalDevice = physicalDevice;
}

// -------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VulkanApp::SwapChainSupportDetails VulkanApp::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VkSurfaceFormatKHR VulkanApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    for (const auto &availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VkPresentModeKHR VulkanApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VkExtent2D VulkanApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool VulkanApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool VulkanApp::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    for ( const char *layerName : validationLayers )
    {
        bool layerFound = false;

        for ( const auto &layerProperties : availableLayers )
        {
            if ( strcmp( layerName, layerProperties.layerName ) == 0 )
            {
                layerFound = true;
                break;
            }
        }

        if ( !layerFound )
        {
            return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
std::vector<const char *> VulkanApp::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    std::vector<const char *> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

    if ( enableValidationLayers )
    {
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    return extensions;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool VulkanApp::isDeviceSuitable(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete()                                                     &&
           swapChainAdequate                                                        &&
           (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ||
            deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)    &&
           deviceFeatures.geometryShader                                            &&
           deviceFeatures.samplerAnisotropy;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VulkanApp::QueueFamilyIndices VulkanApp::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCnt = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCnt, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCnt);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCnt, queueFamilies.data());

    for (uint32_t ii = 0; ii < (uint32_t)queueFamilies.size(); ++ii)
    {
        if (indices.isComplete())
            break;

        if (queueFamilies[ii].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = ii;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, ii, _surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = ii;
        }
    }

    return indices;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
    return 0;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VkShaderModule VulkanApp::createShaderModule(const std::vector<char> &code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createSyncObjects()
{
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    _imagesInFlight.resize(_swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(_device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VkCommandBuffer VulkanApp::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = _commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers( _device, &allocInfo, &commandBuffer );

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer( commandBuffer, &beginInfo );

    return commandBuffer;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::endSingleTimeCommands( VkCommandBuffer commandBuffer )
{
    vkEndCommandBuffer( commandBuffer );

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit( _graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
    vkQueueWaitIdle( _graphicsQueue );

    vkFreeCommandBuffers( _device, _commandPool, 1, &commandBuffer );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createCommandBuffers()
{
    _commandBuffers.resize(_swapChainFramebuffers.size());
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = _commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

    if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    for (size_t i = 0; i < _commandBuffers.size(); i++)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(_commandBuffers[i], &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = _renderPass;
        renderPassInfo.framebuffer = _swapChainFramebuffers[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = _swapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

        VkBuffer vertexBuffers[] = {_vertexBuffer};
        VkDeviceSize offsets[] ={0};
        vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer( _commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT16 );

        vkCmdBindDescriptorSets( _commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 _pipelineLayout, 0, 1, &_descriptorSets[_currentFrame], 0, nullptr );

        vkCmdDrawIndexed( _commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0 );

        vkCmdEndRenderPass(_commandBuffers[i]);

        if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createCommandPool(VkPhysicalDevice physicalDevice)
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    poolInfo.flags = 0; // Optional
    if (vkCreateCommandPool(_device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createBuffer( VkDeviceSize size,
                              VkBufferUsageFlags usage,
                              VkMemoryPropertyFlags properties,
                              VkBuffer &buffer,
                              VkDeviceMemory &bufferMemory )
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ( vkCreateBuffer( _device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS ) {
        throw std::runtime_error( "failed to create buffer!" );
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( _device, buffer, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, properties );

    if ( vkAllocateMemory( _device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate buffer memory!" );
    }

    vkBindBufferMemory( _device, buffer, bufferMemory, 0 );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof( vertices[0] ) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory );

    void *data;
    vkMapMemory( _device, stagingBufferMemory, 0, bufferSize, 0, &data );
    memcpy( data, vertices.data(), (size_t)bufferSize );
    vkUnmapMemory( _device, stagingBufferMemory );

    createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _vertexBuffer, _vertexBufferMemory );

    copyBuffer( stagingBuffer, _vertexBuffer, bufferSize );

    vkDestroyBuffer( _device, stagingBuffer, nullptr );
    vkFreeMemory( _device, stagingBufferMemory, nullptr );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size )
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

    endSingleTimeCommands( commandBuffer );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createImage( uint32_t width,
                             uint32_t height,
                             VkFormat format,
                             VkImageTiling tiling,
                             VkImageUsageFlags usage,
                             VkMemoryPropertyFlags properties,
                             VkImage &image,
                             VkDeviceMemory &imageMemory )
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ( vkCreateImage( _device, &imageInfo, nullptr, &image ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create image!" );
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements( _device, image, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, properties );

    if ( vkAllocateMemory( _device, &allocInfo, nullptr, &imageMemory ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate image memory!" );
    }

    vkBindImageMemory( _device, image, imageMemory, 0 );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, _descriptorSetLayout );
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = _descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    _descriptorSets.resize( MAX_FRAMES_IN_FLIGHT );
    if ( vkAllocateDescriptorSets( _device, &allocInfo, _descriptorSets.data() ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate descriptor sets!" );
    }

    updateDescriptorSets();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createDescriptorPool()
{
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if ( vkCreateDescriptorPool( _device, &poolInfo, nullptr, &_descriptorPool ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create descriptor pool!" );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::updateUniformBuffer( uint32_t currentImage )
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

    time = 2.0f;
    UniformBufferObject ubo{};
    ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );

    // ubo.view = glm::lookAt( glm::vec3( 2.0f, 2.0f, 2.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
    ubo.view = glm::lookAt( glm::vec3( 0.5f, 0.0f, 2.5f ), glm::vec3( 0.5f, 0.0f, 0.0f ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
    ubo.proj = glm::perspective( glm::radians( 45.0f ), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 10.0f );
    ubo.proj[1][1] *= -1;

    memcpy( _uniformBuffersMapped[currentImage], &ubo, sizeof( ubo ) );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof( UniformBufferObject );

    _uniformBuffers.resize( MAX_FRAMES_IN_FLIGHT );
    _uniformBuffersMemory.resize( MAX_FRAMES_IN_FLIGHT );
    _uniformBuffersMapped.resize( MAX_FRAMES_IN_FLIGHT );

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
    {
        createBuffer( bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      _uniformBuffers[i], _uniformBuffersMemory[i] );

        vkMapMemory( _device, _uniformBuffersMemory[i], 0, bufferSize, 0, &_uniformBuffersMapped[i] );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof( indices[0] ) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory );

    void *data;
    vkMapMemory( _device, stagingBufferMemory, 0, bufferSize, 0, &data );
    memcpy( data, indices.data(), (size_t)bufferSize );
    vkUnmapMemory( _device, stagingBufferMemory );

    createBuffer( bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  _indexBuffer, _indexBufferMemory );

    copyBuffer( stagingBuffer, _indexBuffer, bufferSize );

    vkDestroyBuffer( _device, stagingBuffer, nullptr );
    vkFreeMemory( _device, stagingBufferMemory, nullptr );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::transitionImageLayout( VkImage image, VkFormat format,
                                       VkImageLayout oldLayout,
                                       VkImageLayout newLayout )
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else
    {
        throw std::invalid_argument( "unsupported layout transition!" );
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands( commandBuffer );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createTextureImage( const std::string &texImgFile )
{
    int texWidth, texHeight, texChannels;
    stbi_uc *pixels = stbi_load( texImgFile.c_str(),
                                 &texWidth, &texHeight, &texChannels, STBI_rgb_alpha );
    VkDeviceSize imageSize = texWidth * texHeight * 4u;

    if ( !pixels )
    {
        throw std::runtime_error( "failed to load texture image!" );
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory );

    void *data;
    vkMapMemory( _device, stagingBufferMemory, 0, imageSize, 0, &data );
    memcpy( data, pixels, static_cast<size_t>(imageSize) );
    vkUnmapMemory( _device, stagingBufferMemory );

    stbi_image_free( pixels );

    createImage( texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory );

    transitionImageLayout( _textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

    copyBufferToImage( stagingBuffer, _textureImage,
                       static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight) );

    transitionImageLayout( _textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    vkDestroyBuffer( _device, stagingBuffer, nullptr );
    vkFreeMemory( _device, stagingBufferMemory, nullptr );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createTextureImage( const std::vector<unsigned char> &pixels,
                                    uint32_t width, uint32_t height )
{
    if ( pixels.empty() || pixels.size() != width * height * 4 )
    {
        throw std::runtime_error( "invalid texture data!" );
    }

    VkDeviceSize imageSize = pixels.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer( imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer, stagingBufferMemory );

    void *data;
    vkMapMemory( _device, stagingBufferMemory, 0, imageSize, 0, &data );
    memcpy( data, pixels.data(), static_cast<size_t>(imageSize) );
    vkUnmapMemory( _device, stagingBufferMemory );

    createImage( width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _textureImage, _textureImageMemory );

    transitionImageLayout( _textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                           VK_IMAGE_LAYOUT_UNDEFINED,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

    copyBufferToImage( stagingBuffer, _textureImage, width, height );

    transitionImageLayout( _textureImage, VK_FORMAT_R8G8B8A8_SRGB,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );

    vkDestroyBuffer( _device, stagingBuffer, nullptr );
    vkFreeMemory( _device, stagingBufferMemory, nullptr );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createFrameBuffers()
{
    _swapChainFramebuffers.resize(_swapChainImageViews.size());
    for (size_t i = 0; i < _swapChainImageViews.size(); i++)
    {
        VkImageView attachments[] = {_swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _swapChainExtent.width;
        framebufferInfo.height = _swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::copyBufferToImage( VkBuffer buffer, VkImage image,
                                   uint32_t width, uint32_t height )
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = {
        width,
        height,
        1
    };
    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
    endSingleTimeCommands( commandBuffer );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    auto bindingDescription    = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_swapChainExtent.width;
    viewport.height = (float)_swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = _swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;

    if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr; // Optional
    pipelineInfo.layout = _pipelineLayout;
    pipelineInfo.renderPass = _renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1;              // Optional

    if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
VkImageView VulkanApp::createImageView( VkImage image, VkFormat format )
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if ( vkCreateImageView( _device, &viewInfo, nullptr, &imageView ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create texture image view!" );
    }

    return imageView;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::updateDescriptorSets()
{
    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = _uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( UniformBufferObject );

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = _textureImageView;
        imageInfo.sampler = _textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = _descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = _descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets( _device, static_cast<uint32_t>(descriptorWrites.size()),
                                descriptorWrites.data(), 0, nullptr );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createImageViews()
{
    _swapChainImageViews.resize(_swapChainImages.size());
    for (size_t i = 0; i < _swapChainImages.size(); i++)
    {
        _swapChainImageViews[i] = createImageView( _swapChainImages[i], _swapChainImageFormat );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createSwapChain(VkPhysicalDevice physicalDevice)
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
    _swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

    _swapChainExtent = extent;
    _swapChainImageFormat = surfaceFormat.format;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createTextureImageView()
{
    _textureImageView = createImageView( _textureImage, VK_FORMAT_R8G8B8A8_SRGB );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::cleanupImageTexture()
{
    vkDestroyImageView( _device, _textureImageView, nullptr );
    vkDestroyImage( _device, _textureImage, nullptr );
    vkFreeMemory( _device, _textureImageMemory, nullptr );
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createTextureSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties( _physicalDevice, &properties );

    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if ( vkCreateSampler( _device, &samplerInfo, nullptr, &_textureSampler ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create texture sampler!" );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createLogicalDevice(VkPhysicalDevice physicalDevice)
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createInstance()
{
    if ( enableValidationLayers && !checkValidationLayerSupport() )
    {
        throw std::runtime_error( "validation layers requested, but not available!" );
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if ( enableValidationLayers )
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
        populateDebugMessengerCreateInfo( debugCreateInfo );
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vulkan instance!");
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::mainLoop()
{
    while (!glfwWindowShouldClose(_window))
    {
        glfwPollEvents();
        drawFrame();
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::createDiscriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if ( vkCreateDescriptorSetLayout( _device, &layoutInfo, nullptr, &_descriptorSetLayout ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create descriptor set layout!" );
    }
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::drawFrame()
{
    vkWaitForFences(_device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(_device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (_imagesInFlight[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(_device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    _imagesInFlight[imageIndex] = _inFlightFences[_currentFrame];

    updateUniformBuffer( static_cast<uint32_t>(_currentFrame) );

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(_device, 1, &_inFlightFences[_currentFrame]);

    if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {_swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(_presentQueue, &presentInfo);

    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void VulkanApp::cleanup()
{
    if ( enableValidationLayers )
    {
        DestroyDebugUtilsMessengerEXT( _instance, _debugMessenger, nullptr );
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(_device, _renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(_device, _imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(_device, _inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(_device, _commandPool, nullptr);

    for (auto framebuffer : _swapChainFramebuffers)
    {
        vkDestroyFramebuffer(_device, framebuffer, nullptr);
    }

    vkDestroyPipeline(_device, _graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);
    vkDestroyRenderPass(_device, _renderPass, nullptr);

    for (auto imageView : _swapChainImageViews)
    {
        vkDestroyImageView(_device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(_device, _swapChain, nullptr);

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
    {
        vkDestroyBuffer( _device, _uniformBuffers[i], nullptr );
        vkFreeMemory( _device, _uniformBuffersMemory[i], nullptr );
    }

    vkDestroyDescriptorPool( _device, _descriptorPool, nullptr );
    vkDestroyDescriptorSetLayout( _device, _descriptorSetLayout, nullptr );

    vkDestroyBuffer( _device, _indexBuffer, nullptr );
    vkFreeMemory( _device, _indexBufferMemory, nullptr );
    vkDestroyBuffer(_device, _vertexBuffer, nullptr);
    vkFreeMemory(_device, _vertexBufferMemory, nullptr);

    vkDestroySampler( _device, _textureSampler, nullptr );
    cleanupImageTexture();

    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
    glfwDestroyWindow(_window);
    glfwTerminate();
}
