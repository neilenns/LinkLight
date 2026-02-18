# About esp32-psram

This code is from https://github.com/pschatzmann/esp32-psram. `HimemBlock.h` was modified to force it to use
`#include "esp32/himem.h"` to get rid of compile time warnings about a deprecated `himem.h` file.
