#ifndef PSRAMSTRING_H
#define PSRAMSTRING_H

#include <string>
#include "esp32-psram/AllocatorPSRAM.h"

// PSRAM-backed string type
using PSRAMString = std::basic_string<char, std::char_traits<char>, esp32_psram::AllocatorPSRAM<char>>;

#endif // PSRAMSTRING_H
