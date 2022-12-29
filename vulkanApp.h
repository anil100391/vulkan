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
struct WindowParams
{
    uint32_t width{ 1024 };
    uint32_t height{ 1024 };
    std::string title{ "Vulkan App" };
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class VulkanApp
{
public:

    VulkanApp() = default;
    virtual ~VulkanApp() = default;

    virtual WindowParams GetWindowParams() const;

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
    VkCommandBuffer             beginSingleTimeCommands();
    void                        endSingleTimeCommands( VkCommandBuffer commandBuffer );
    void                        createCommandBuffers();
    void                        createCommandPool(VkPhysicalDevice physicalDevice);
    void                        createBuffer( VkDeviceSize size,
                                              VkBufferUsageFlags usage,
                                              VkMemoryPropertyFlags properties,
                                              VkBuffer &buffer,
                                              VkDeviceMemory &bufferMemory );
    void                        createVertexBuffer();
    void                        createIndexBuffer();
    void                        createUniformBuffers();
    void                        createDescriptorPool();
    void                        createDescriptorSets();
    void                        updateUniformBuffer( uint32_t currentImage );
    void                        copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );
    void                        createImage( uint32_t width, uint32_t height, VkFormat format,
                                             VkImageTiling tiling, VkImageUsageFlags usage,
                                             VkMemoryPropertyFlags properties, VkImage &image,
                                             VkDeviceMemory &imageMemory );
    void                        transitionImageLayout( VkImage image, VkFormat format,
                                                       VkImageLayout oldLayout,
                                                       VkImageLayout newLayout );
    void                        copyBufferToImage( VkBuffer buffer, VkImage image,
                                                   uint32_t width, uint32_t height );
    void                        createTextureImage();
    void                        createTextureImageView();
    void                        createTextureSampler();
    void                        createFrameBuffers();
    void                        createDiscriptorSetLayout();
    void                        createGraphicsPipeline();
    VkImageView                 createImageView( VkImage image, VkFormat format );
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
    VkBuffer                        _indexBuffer;
    VkDeviceMemory                  _indexBufferMemory;
    VkImage                         _textureImage;
    VkDeviceMemory                  _textureImageMemory;
    VkImageView                     _textureImageView;
    VkSampler                       _textureSampler;
    VkDescriptorSetLayout           _descriptorSetLayout;
    VkDescriptorPool                _descriptorPool;
    std::vector<VkDescriptorSet>    _descriptorSets;
    std::vector<VkBuffer>           _uniformBuffers;
    std::vector<VkDeviceMemory>     _uniformBuffersMemory;
    std::vector<void *>             _uniformBuffersMapped;
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

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
inline WindowParams VulkanApp::GetWindowParams() const
{
    return WindowParams{};
}
