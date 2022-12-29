#include <cmath>
#include <chrono>
#include <thread>
#include <iostream>
#include <algorithm>

#include "fractals.h"
#include <fstream>

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool nhImage::PointAtPixel( const int px, const int py,
                            double &x, double &y,
                            const nhImage::PIXEL_CORNER loc ) const
{
    if ( px < 0 || px > p_resX || py < 0 || py > p_resY )
    {
        return false;
    }

    double pixelSpanX = (p_xMax - p_xMin) / p_resX;
    x = p_xMin + px * pixelSpanX;

    double pixelSpanY = (p_yMax - p_yMin) / p_resY;
    y = p_yMin + py * pixelSpanY;

    switch ( loc )
    {
    case CENTER:
        x += 0.5 * pixelSpanX;
        y += 0.5 * pixelSpanY;
        break;
    case LOWER_LEFT:
        y += pixelSpanY;
        break;
    case LOWER_RIGHT:
        x += pixelSpanX;
        y += pixelSpanY;
        break;
    case UPPER_RIGHT:
        x += pixelSpanX;
        break;
    case UPPER_LEFT:
        break;
    default:
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool nhImage::PixelAtPoint( const double x, const double y, int &px, int &py )
const
{
    if ( x > p_xMax || x < p_xMin || y > p_yMax || y < p_yMin )
    {
        return false;
    }

    double pixelSpan = (p_xMax - p_xMin) / (p_resX - 1);
    px = static_cast<int>((x - p_xMin) / pixelSpan);

    pixelSpan = (p_yMax - p_yMin) / (p_resY - 1);
    py = static_cast<int>((y - p_yMin) / pixelSpan);

    return true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
bool nhImage::Paint( void )
{
    if ( p_colorData.empty() )
    {
        p_colorData.resize( 4 * p_resX * p_resY, 0 );
        return true;
    }

    return false;
}

// -------------------------------------------------------------------------- //
// -------------------------------------------------------------------------- //
int nhNebulabrot::IterationsToGetKnocked( int row, int col ) const
{
    int count = 0;
    double cx = 0.0, cy = 0.0, x0 = 0.0, y0 = 0.0;
    nhImage::PointAtPixel( row, col, cx, cy, nhImage::CENTER );
    // Iteration of 0 under f(z) = z^2 + c //
    while ( x0 * x0 + y0 * y0 < 4 && count < p_maxIter )
    {
        double fx = x0 * x0 - y0 * y0 + cx;
        double fy = 2.0 * x0 * y0 + cy;
        x0 = fx;
        y0 = fy;
        ++count;
    }
    return count;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void nhNebulabrot::GetAStartingPoint( double &x, double &y ) const
{
    x = 0.0;
    y = 0.0;

    do
    {
        x = rand() / (float)RAND_MAX;
        y = rand() / (float)RAND_MAX;
        x = (p_xMax - p_xMin) * x + p_xMin;
        y = (p_yMax - p_yMin) * y + p_yMin;
    } while ( !PointHasOrbitBetween( x, y, p_minIter, p_maxIter ) );
}

// -------------------------------------------------------------------------- //
// -------------------------------------------------------------------------- //
bool nhNebulabrot::PointHasOrbitBetween( double cx,
                                         double cy,
                                         int minIter,
                                         int maxIter )
{
    bool circle_cond = (cx + 1) * (cx + 1) + cy * cy < 0.0625;

    if ( circle_cond )
    {
        return false;
    }

    double p = std::sqrt( (cx - 0.25) * (cx - 0.25) + cy * cy );
    bool cardioid_cond = cx - (p - 2 * p * p + 0.25) < 0;

    if ( cardioid_cond )
    {
        return false;
    }

    double x0 = 0.0, y0 = 0.0;
    int count = 0;
    // Iteration of 0 under f(z) = z^2 + c //
    while ( x0 * x0 + y0 * y0 < 4 && count < maxIter )
    {
        double fx = x0 * x0 - y0 * y0 + cx;
        double fy = 2.0 * x0 * y0 + cy;
        x0 = fx;
        y0 = fy;
        ++count;
    }

    return (count >= minIter) && (count < maxIter);
}

// -------------------------------------------------------------------------- //
// -------------------------------------------------------------------------- //
bool nhNebulabrot::Paint( void )
{
    clock_t seed = clock();
    srand( seed );

    std::ofstream outfile( "nebulabrot_points.txt" );
    while ( true && !p_paused )
    {
        double cx = 0, cy = 0;
        GetAStartingPoint( cx, cy );
        outfile << cx << "\t" << cy << "\n";

        // Iteration of 0 under f(z) = z^2 + c //
        int count = 0;
        double x0 = 0.0, y0 = 0.0;
        while ( x0 * x0 + y0 * y0 < 4 && count < p_maxIter )
        {
            ++count;

            double fx = x0 * x0 - y0 * y0 + cx;
            double fy = 2.0 * x0 * y0 + cy;
            x0 = fx;
            y0 = fy;

            int px = 0, py = 0;
            if ( nhImage::PixelAtPoint( x0, y0, px, py ) )
            {
                // increase pixel brightness
                uint8_t *pixel = &p_colorData[4 * (py * p_resX + px)];
                uint32_t &hitCount = *reinterpret_cast<uint32_t *>(pixel);
                ++hitCount;
            }
        }
    }

    return true;
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
struct PaintJob
{
    PaintJob( nhNebulabrot *fractal ) : _fractal( fractal ) {}

    void operator()()
    {
        _fractal->Paint();
    }

    nhNebulabrot *_fractal = nullptr;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
FractalsApp::FractalsApp()
{
    auto wp = GetWindowParams();
    auto width = wp.width;
    auto height = wp.height;

    int minIter = 25;
    int maxIter = 1000;
    _fractal = std::make_unique<nhNebulabrot>( -2.0f, 1.0f, -1.0f, 1.0f, width, height, maxIter, minIter );

    StartPainting();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
FractalsApp::~FractalsApp()
{
    PausePainting();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void FractalsApp::StartPainting()
{
    _fractal->PausePaint( false );
    assert( _paintJobs.empty() );
    _paintJobs.emplace_back( PaintJob( _fractal.get() ) );
    _paintJobs[0].detach();
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void FractalsApp::PausePainting()
{
    _fractal->PausePaint( true );
    _paintJobs.clear();
}

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

    PausePainting();

    auto p_colorData = _fractal->Pixels();

    uint32_t maxHits = 0;
    for ( size_t ii = 0; ii < p_colorData.size() / 4; ++ii )
    {
        uint8_t *pixel = &p_colorData[4 * ii];
        uint32_t hitCount = *reinterpret_cast<uint32_t *>(pixel);
        if ( hitCount > maxHits )
        {
            maxHits = hitCount;
        }
    }

    if ( maxHits != 0 )
    {
        for ( size_t ii = 0; ii < p_colorData.size() / 4; ++ii )
        {
            uint8_t *pixel = &p_colorData[4 * ii];
            uint32_t hitCount = *reinterpret_cast<uint32_t *>(pixel);
            if ( hitCount == 0 )
            {
                pixel[3] = 255;
                continue;
            }
            float density = 1.0f * hitCount / maxHits;
            density = std::pow( density, 0.85f );
            // density *= 2.0f;
            density = std::clamp( density, 0.0f, 1.0f );
            uint8_t intensity = static_cast<uint8_t>(255 * density);

            if ( intensity != 0 )
            {
                pixel[0] = intensity;
                pixel[1] = intensity;
                pixel[2] = intensity;
            }
            pixel[3] = 255;
        }
    }

    cleanupImageTexture();
    createTextureImage( p_colorData, width, height );
    createTextureImageView();
    updateDescriptorSets();
    vkFreeCommandBuffers( _device, _commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data() );
    createCommandBuffers();

    StartPainting();
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