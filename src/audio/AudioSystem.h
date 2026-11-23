#pragma once
#include <cstdint>

namespace fowe {

bool        AudioInit();
void        AudioShutdown();
const char* AudioBackendName();
uint32_t    AudioSampleRate();

} // namespace fowe
