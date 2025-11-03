#include <X11/X.h>
#include <X11/Xlib.h>

class Device {
public:
    Device();
    int GetScreen(int index=0);
    int GetScreenWidth(int index=0);
    int GetScreenHeight(int index=0);
    ~Device();
private:
    Display* display;

    // TODO: multihead
    // TODO: sound inputs
};
