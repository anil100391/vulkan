#include <atomic>
#include <vector>
#include <thread>
#include <cassert>

#include "../vulkanApp.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class nhImage
{
public:

    nhImage() = delete;

    nhImage( double xmin, double xmax,
             double ymin, double ymax,
             int resX, int resY )
        : p_xMin( xmin ),
          p_xMax( xmax ),
          p_yMin( ymin ),
          p_yMax( ymax ),
          p_resX( resX ),
          p_resY( resY )
    {
        Paint();
    }

    virtual ~nhImage() = default;

    enum PIXEL_CORNER
    {
        CENTER,
        LOWER_LEFT,
        LOWER_RIGHT,
        UPPER_RIGHT,
        UPPER_LEFT,
    };

    // initializes pixels of the image
    virtual bool Paint( void );

    // Gets the point (x,y) lying on pixel px, py which
    // corresponds to coordinate of the provided pixel corner
    bool PointAtPixel( const int px, const int py,
                       double &x, double &y,
                       const PIXEL_CORNER loc = CENTER ) const;

    // Gets the pixel lying on point
    bool PixelAtPoint( const double x, const double y,
                       int &px, int &py ) const;

    std::vector<uint8_t> &Pixels() { return p_colorData; }

protected:

    // pixel color info
    std::vector<uint8_t> p_colorData;

    // Geometry corresponding to image
    double  p_xMin;
    double  p_xMax;
    double  p_yMin;
    double  p_yMax;

    // Image resolution
    int     p_resX;
    int     p_resY;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class nhNebulabrot : public nhImage
{
public:

    nhNebulabrot( double xmin, double xmax,
                  double ymin, double ymax,
                  int resX, int resY, int maxIter, int minIter )
        : nhImage( xmin, xmax, ymin, ymax, resX, resY ),
          p_maxIter( maxIter ),
          p_minIter( minIter )
    {
        assert( minIter < maxIter );
    }

    virtual ~nhNebulabrot() = default;

    // Manipulates the color space so that to represent each pixel
    // belonging or not belonging to the mandelbrot set.
    bool Paint( void ) override;

    void PausePaint( bool flag ) { p_paused = flag; }

    std::vector<unsigned char> GetHeatPlot() const;
private:

    int IterationsToGetKnocked( int row, int col ) const;
    static bool PointHasOrbitBetween( double cx,
                                      double cy,
                                      int minIter,
                                      int maxIter );
    void GetAStartingPoint( double &x, double &y ) const;

    int p_maxIter;
    int p_minIter;

    std::atomic_bool p_paused = false;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class FractalsApp : public VulkanApp
{
public:

    FractalsApp();
    virtual ~FractalsApp();

    virtual WindowParams GetWindowParams() const override;

protected:

    virtual void drawFrame() override;

private:

    void PausePainting();
    void StartPainting();

    void UpdatePixels( float time );

    std::unique_ptr<nhNebulabrot> _fractal;
    std::vector<std::thread>      _paintJobs;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
inline WindowParams FractalsApp::GetWindowParams() const
{
    WindowParams wp{ 1200, 800, "Buddhabrot" };
    return wp;
}
