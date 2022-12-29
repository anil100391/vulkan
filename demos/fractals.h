#include "../vulkanApp.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class FractalsApp : public VulkanApp
{
public:

    FractalsApp() = default;
    virtual ~FractalsApp() = default;

    virtual WindowParams GetWindowParams() const override;

protected:

    virtual void drawFrame() override;

private:

    void UpdatePixels( float time );

    std::vector<unsigned char> p_pixels;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
inline WindowParams FractalsApp::GetWindowParams() const
{
    WindowParams wp{ 1200, 800, "Buddhabrot" };
    return wp;
}
