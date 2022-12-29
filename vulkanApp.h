#ifdef WIN32
    #define VK_USE_PLATFORM_WIN32_KHR
#endif
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3native.h>

#include <vector>

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class VulkanApp
{
public:

    VulkanApp() = default;
    virtual ~VulkanApp() = default;

    void run();

protected:

    struct QueueFamilyIndices;
    struct SwapChainSupportDetails;

    void                        initWindow();
    void                        initVulkan();
    void                        setupDebugMessenger();
    void                        createSurface();
    void                        pickPhysicalDevice();
    SwapChainSupportDetails     querySwapChainSupport( VkPhysicalDevice device );
    VkSurfaceFormatKHR          chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats );
    VkPresentModeKHR            chooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes );
    VkExtent2D                  chooseSwapExtent( const VkSurfaceCapabilitiesKHR &capabilities );
    bool                        checkDeviceExtensionSupport( VkPhysicalDevice device );
    bool                        isDeviceSuitable( VkPhysicalDevice device );
    QueueFamilyIndices          findQueueFamilies( VkPhysicalDevice device );
    uint32_t                    findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void                        createRenderPass();
    VkShaderModule              createShaderModule( const std::vector<char> &code );
    void                        createSyncObjects();
    void                        createCommandBuffers();
    void                        createCommandPool(VkPhysicalDevice physicalDevice);
    void                        createVertexBuffer();
    void                        createFrameBuffers();
    void                        createGraphicsPipeline();
    void                        createImageViews();
    void                        createSwapChain(VkPhysicalDevice physicalDevice);
    void                        createLogicalDevice(VkPhysicalDevice physicalDevice);
    void                        createInstance();
    void                        mainLoop();
    void                        drawFrame();
    void                        cleanup();

    GLFWwindow*                     _window = nullptr;
    VkInstance                      _instance;
    VkSurfaceKHR                    _surface;
    VkPhysicalDevice                _physicalDevice; 
    VkDevice                        _device;
    VkQueue                         _graphicsQueue;
    VkQueue                         _presentQueue;
    VkSwapchainKHR                  _swapChain;
    VkBuffer                        _vertexBuffer;
    VkDeviceMemory                  _vertexBufferMemory;
    std::vector<VkImage>            _swapChainImages;
    VkFormat                        _swapChainImageFormat;
    VkExtent2D                      _swapChainExtent;
    std::vector<VkImageView>        _swapChainImageViews;
    VkPipelineLayout                _pipelineLayout;
    VkRenderPass                    _renderPass;
    VkPipeline                      _graphicsPipeline;
    std::vector<VkFramebuffer>      _swapChainFramebuffers;
    VkCommandPool                   _commandPool;
    std::vector<VkCommandBuffer>    _commandBuffers;
    std::vector<VkSemaphore>        _imageAvailableSemaphores;
    std::vector<VkSemaphore>        _renderFinishedSemaphores;
    std::vector<VkFence>            _inFlightFences;
    std::vector<VkFence>            _imagesInFlight;
    size_t                          _currentFrame = 0;
};
