#include <chrono>
#include <iostream>

#include "fractals.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void FractalsApp::drawFrame()
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

    static float lastUpdateTime = 0.0f;

    if ( time > lastUpdateTime + 1.0f )
    {
        UpdatePixels( time );
        lastUpdateTime = time;
    }

    VulkanApp::drawFrame();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void FractalsApp::UpdatePixels( float time )
{
    auto wp = GetWindowParams();
    uint32_t width = wp.width;
    uint32_t height = wp.height;

    if ( p_pixels.empty() )
    {
        p_pixels.resize( width * height * 4u, 0 );
    }
    else
    {
        unsigned char value = 2 * time > 255 ? 255 : 2 * time;
        std::fill_n( p_pixels.begin(), p_pixels.size(), value );
    }

    cleanupImageTexture();
    createTextureImage( p_pixels, width, height );
    createTextureImageView();
    updateDescriptorSets();
    vkFreeCommandBuffers( _device, _commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data() );
    createCommandBuffers();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
int main()
{
    FractalsApp app;

    try
    {
        app.run();
    }
    catch ( const std::exception &e )
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}