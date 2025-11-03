#include "device.hpp"
#include <X11/Xlib.h>

Device::Device() {
    display = XOpenDisplay(NULL);
}

int Device::GetScreen(int index) {
    return XDefaultRootWindow(display);
}

int Device::GetScreenWidth(int index) {
    return DisplayWidth(display, 0);
}

int Device::GetScreenHeight(int index) {
    return DisplayHeight(display, 0);
}

Device::~Device() {
    XCloseDisplay(display);
}
