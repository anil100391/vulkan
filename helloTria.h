#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vector>
#include <iostream>
#include <stdexcept>
#include <optional>

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

        bool isComplete() const
        {
            return graphicsFamily.has_value();
        }
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

    bool isDeviceSuitable( VkPhysicalDevice device )
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties( device, &deviceProperties );

        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures( device, &deviceFeatures );

        QueueFamilyIndices indices = findQueueFamilies( device );

        return indices.isComplete()                                                 &&
               deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU  &&
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
        }

        return indices;
    }

    void createLogicalDevice(VkPhysicalDevice physicalDevice)
    {
        QueueFamilyIndices indices = findQueueFamilies( physicalDevice );
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};
        vkGetPhysicalDeviceFeatures( physicalDevice, &deviceFeatures );

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;

        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = 0;
        createInfo.enabledLayerCount = 0;

        if ( vkCreateDevice( physicalDevice, &createInfo, nullptr, &_device ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create logical device!" );
        }

        vkGetDeviceQueue( _device, indices.graphicsFamily.value(), 0, &_graphicsQueue );
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
        }
    }

    void cleanup()
    {
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
};