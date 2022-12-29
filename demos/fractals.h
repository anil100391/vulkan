#include "../vulkanApp.h"

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
class FractalsApp : public VulkanApp
{
public:

    FractalsApp() = default;
    virtual ~FractalsApp() = default;

    virtual WindowParams GetWindowParams() const override;
};

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
inline WindowParams FractalsApp::GetWindowParams() const
{
    WindowParams wp{ 1200, 800, "Buddhabrot" };
    return wp;
}
