#ifdef WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

#include <set>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <optional>
#include <algorithm>
#include <fstream>

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
const int MAX_FRAMES_IN_FLIGHT = 2;

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
const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class HelloTriangleApp
{
public:

    void run()
    {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() const
        {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    void initWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
        _window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    }

    void initVulkan()
    {
        createInstance();
        setupDebugMessenger();
        createSurface();
        VkPhysicalDevice physicalDevice = pickPhysicalDevice();
        createLogicalDevice( physicalDevice );
        createSwapChain( physicalDevice );
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFrameBuffers();
        createCommandPool( physicalDevice );
        createCommandBuffers();
        createSyncObjects();
    }

    void setupDebugMessenger()
    {
    }

    void createSurface()
    {
        if ( glfwCreateWindowSurface( _instance, _window, nullptr, &_surface ) != VK_SUCCESS )
            throw std::runtime_error( "failed to create window surface!" );
    }

    VkPhysicalDevice pickPhysicalDevice()
    {
        uint32_t deviceCnt = 0;
        vkEnumeratePhysicalDevices( _instance, &deviceCnt, nullptr );
        if ( deviceCnt == 0 )
            throw std::runtime_error( "failed to find GPUs with Vulkan support!" );

        std::vector<VkPhysicalDevice> devices( deviceCnt );
        vkEnumeratePhysicalDevices( _instance, &deviceCnt, devices.data() );

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

        for ( const auto& device : devices )
        {
            if ( isDeviceSuitable( device ) )
            {
                physicalDevice = device;
                break;
            }
        }

        if ( physicalDevice == VK_NULL_HANDLE )
        {
            throw std::runtime_error( "failed to find a suitable GPU!" );
        }

        return physicalDevice;
    }

    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device )
    {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, _surface, &details.capabilities );

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, _surface, &formatCount, nullptr );

        if ( formatCount != 0 )
        {
            details.formats.resize( formatCount );
            vkGetPhysicalDeviceSurfaceFormatsKHR( device, _surface, &formatCount, details.formats.data() );
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, _surface, &presentModeCount, nullptr );

        if ( presentModeCount != 0 )
        {
            details.presentModes.resize( presentModeCount );
            vkGetPhysicalDeviceSurfacePresentModesKHR( device, _surface, &presentModeCount, details.presentModes.data() );
        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats )
    {
        for ( const auto &availableFormat : availableFormats )
        {
            if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
            {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes )
    {
        for ( const auto &availablePresentMode : availablePresentModes )
        {
            if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
            {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR &capabilities )
    {
        if ( capabilities.currentExtent.width != UINT32_MAX )
        {
            return capabilities.currentExtent;
        }

        int width, height;
        glfwGetFramebufferSize( _window, &width, &height );

        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actualExtent.width = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
        actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

        return actualExtent;
    }

    bool checkDeviceExtensionSupport( VkPhysicalDevice device )
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

        std::vector<VkExtensionProperties> availableExtensions( extensionCount );
        vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

        std::set<std::string> requiredExtensions( deviceExtensions.begin(), deviceExtensions.end() );

        for ( const auto &extension : availableExtensions ) {
            requiredExtensions.erase( extension.extensionName );
        }

        return requiredExtensions.empty();
    }

    bool isDeviceSuitable( VkPhysicalDevice device )
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties( device, &deviceProperties );

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures( device, &deviceFeatures );

        QueueFamilyIndices indices = findQueueFamilies( device );

        bool extensionsSupported = checkDeviceExtensionSupport( device );
        bool swapChainAdequate = false;
        if ( extensionsSupported )
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport( device );
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete()                                                  &&
               swapChainAdequate                                                     &&
               (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU ||
                deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) &&
               deviceFeatures.geometryShader;
    }

    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device )
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCnt = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCnt, nullptr );

        std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCnt );
        vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCnt, queueFamilies.data() );

        for ( uint32_t ii = 0; ii < (uint32_t)queueFamilies.size(); ++ii )
        {
            if ( indices.isComplete() )
                break;

            if ( queueFamilies[ii].queueFlags & VK_QUEUE_GRAPHICS_BIT )
            {
                indices.graphicsFamily = ii;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR( device, ii, _surface, &presentSupport );
            if ( presentSupport )
            {
                indices.presentFamily = ii;
            }
        }

        return indices;
    }

    void createRenderPass()
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

        if ( vkCreateRenderPass( _device, &renderPassInfo, nullptr, &_renderPass ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create render pass!" );
        }

    }

    VkShaderModule createShaderModule( const std::vector<char> &code )
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if ( vkCreateShaderModule( _device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create shader module!" );
        }

        return shaderModule;
    }

    void createSyncObjects()
    {
        _imageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
        _renderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
        _inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );
        _imagesInFlight.resize( _swapChainImages.size(), VK_NULL_HANDLE );

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
            if ( vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i] ) != VK_SUCCESS ||
                 vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i] ) != VK_SUCCESS ||
                 vkCreateFence( _device, &fenceInfo, nullptr, &_inFlightFences[i] ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to create synchronization objects for a frame!" );
            }
        }
    }

    void createCommandBuffers()
    {
        _commandBuffers.resize( _swapChainFramebuffers.size() );
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = _commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

        if ( vkAllocateCommandBuffers( _device, &allocInfo, _commandBuffers.data() ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to allocate command buffers!" );
        }

        for ( size_t i = 0; i < _commandBuffers.size(); i++ ) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if ( vkBeginCommandBuffer( _commandBuffers[i], &beginInfo ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to begin recording command buffer!" );
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = _renderPass;
            renderPassInfo.framebuffer = _swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = _swapChainExtent;

            VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass( _commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

            vkCmdBindPipeline( _commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline );

            vkCmdDraw( _commandBuffers[i], 3, 1, 0, 0 );

            vkCmdEndRenderPass( _commandBuffers[i] );

            if ( vkEndCommandBuffer( _commandBuffers[i] ) != VK_SUCCESS ) {
                throw std::runtime_error( "failed to record command buffer!" );
            }
        }
    }

    void createCommandPool(VkPhysicalDevice physicalDevice)
    {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies( physicalDevice );

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
        poolInfo.flags = 0; // Optional
        if ( vkCreateCommandPool( _device, &poolInfo, nullptr, &_commandPool ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create command pool!" );
        }
    }

    void createFrameBuffers() 
    {
        _swapChainFramebuffers.resize( _swapChainImageViews.size() );
        for ( size_t i = 0; i < _swapChainImageViews.size(); i++ )
        {
            VkImageView attachments[] = { _swapChainImageViews[i] };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = _renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = _swapChainExtent.width;
            framebufferInfo.height = _swapChainExtent.height;
            framebufferInfo.layers = 1;

            if ( vkCreateFramebuffer( _device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i] ) != VK_SUCCESS )
            {
                throw std::runtime_error( "failed to create framebuffer!" );
            }
        }
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = readFile( "shaders/vert.spv" );
        auto fragShaderCode = readFile( "shaders/frag.spv" );

        VkShaderModule vertShaderModule = createShaderModule( vertShaderCode );
        VkShaderModule fragShaderModule = createShaderModule( fragShaderCode );

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

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

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
        scissor.offset = { 0, 0 };
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
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;

        if ( vkCreatePipelineLayout( _device, &pipelineLayoutInfo, nullptr, &_pipelineLayout ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to create pipeline layout!" );
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
        pipelineInfo.basePipelineIndex = -1; // Optional

        if ( vkCreateGraphicsPipelines( _device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create graphics pipeline!" );
        }

        vkDestroyShaderModule( _device, fragShaderModule, nullptr );
        vkDestroyShaderModule( _device, vertShaderModule, nullptr );
    }

    void createImageViews()
    {
        _swapChainImageViews.resize( _swapChainImages.size() );
        for ( size_t i = 0; i < _swapChainImages.size(); i++ )
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = _swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = _swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;
            if ( vkCreateImageView( _device, &createInfo, nullptr, &_swapChainImageViews[i] ) != VK_SUCCESS )
            {
                throw std::runtime_error( "failed to create image views!" );
            }
        }
    }

    void createSwapChain(VkPhysicalDevice physicalDevice)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport( physicalDevice );

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
        VkPresentModeKHR presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
        VkExtent2D extent = chooseSwapExtent( swapChainSupport.capabilities );

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
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

        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );
        uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        if ( indices.graphicsFamily != indices.presentFamily )
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if ( vkCreateSwapchainKHR( _device, &createInfo, nullptr, &_swapChain ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create swap chain!" );
        }

        vkGetSwapchainImagesKHR( _device, _swapChain, &imageCount, nullptr );
        _swapChainImages.resize( imageCount );
        vkGetSwapchainImagesKHR( _device, _swapChain, &imageCount, _swapChainImages.data() );

        _swapChainExtent = extent;
        _swapChainImageFormat = surfaceFormat.format;
    }

    void createLogicalDevice(VkPhysicalDevice physicalDevice)
    {
        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

        float queuePriority = 1.0f;
        for ( uint32_t queueFamily : uniqueQueueFamilies )
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back( queueCreateInfo );
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures( physicalDevice, &deviceFeatures );

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();
        createInfo.enabledLayerCount = 0;

        if ( vkCreateDevice( physicalDevice, &createInfo, nullptr, &_device ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create logical device!" );
        }

        vkGetDeviceQueue( _device, indices.graphicsFamily.value(), 0, &_graphicsQueue );
        vkGetDeviceQueue( _device, indices.presentFamily.value(), 0, &_presentQueue );
    }

    void createInstance()
    {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        createInfo.enabledLayerCount = 0;

        if ( vkCreateInstance( &createInfo, nullptr, &_instance ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create vulkan instance!" );
        }
    }

    void mainLoop()
    {
        while ( !glfwWindowShouldClose( _window ) )
        {
            glfwPollEvents();
            drawFrame();
        }
    }

    void drawFrame()
    {
        vkWaitForFences( _device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX );

        uint32_t imageIndex;
        vkAcquireNextImageKHR( _device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex );

        if ( _imagesInFlight[imageIndex] != VK_NULL_HANDLE ) {
            vkWaitForFences( _device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX );
        }
        _imagesInFlight[imageIndex] = _inFlightFences[_currentFrame];

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[_currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[_currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences( _device, 1, &_inFlightFences[_currentFrame] );

        if ( vkQueueSubmit( _graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame] ) != VK_SUCCESS ) {
            throw std::runtime_error( "failed to submit draw command buffer!" );
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = { _swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR( _presentQueue, &presentInfo );

        _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanup()
    {
        for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ ) {
            vkDestroySemaphore( _device, _renderFinishedSemaphores[i], nullptr );
            vkDestroySemaphore( _device, _imageAvailableSemaphores[i], nullptr );
            vkDestroyFence( _device, _inFlightFences[i], nullptr );
        }

        vkDestroyCommandPool( _device, _commandPool, nullptr );

        for ( auto framebuffer : _swapChainFramebuffers )
        {
            vkDestroyFramebuffer( _device, framebuffer, nullptr );
        }

        vkDestroyPipeline( _device, _graphicsPipeline, nullptr );
        vkDestroyPipelineLayout( _device, _pipelineLayout, nullptr );
        vkDestroyRenderPass( _device, _renderPass, nullptr );

        for ( auto imageView : _swapChainImageViews )
        {
            vkDestroyImageView( _device, imageView, nullptr );
        }

        vkDestroySwapchainKHR( _device, _swapChain, nullptr );
        vkDestroyDevice( _device, nullptr );
        vkDestroySurfaceKHR( _instance, _surface, nullptr );
        vkDestroyInstance( _instance, nullptr );
        glfwDestroyWindow( _window );
        glfwTerminate();
    }

    GLFWwindow*     _window = nullptr;
    VkInstance      _instance;
    VkSurfaceKHR    _surface;
    VkDevice        _device;
    VkQueue         _graphicsQueue;
    VkQueue         _presentQueue;
    VkSwapchainKHR  _swapChain;
    std::vector<VkImage> _swapChainImages;
    VkFormat        _swapChainImageFormat;
    VkExtent2D      _swapChainExtent;
    std::vector<VkImageView> _swapChainImageViews;
    VkPipelineLayout _pipelineLayout;
    VkRenderPass     _renderPass;
    VkPipeline       _graphicsPipeline;
    std::vector<VkFramebuffer> _swapChainFramebuffers;
    VkCommandPool    _commandPool;
    std::vector<VkCommandBuffer> _commandBuffers;
    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    std::vector<VkFence> _imagesInFlight;
    size_t _currentFrame = 0;
};
