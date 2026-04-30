#pragma once

#include <thread>

namespace tinyray {

inline int defaultThreadCount()
{
    const unsigned int count = std::thread::hardware_concurrency();
    return count == 0 ? 1 : static_cast<int>(count);
}

class RenderSettings
{
public:
    int width = 800;
    int height = 450;
    int samplesPerPixel = 16;
    int maxDepth = 5;
    int threadCount = defaultThreadCount();
};

} // namespace tinyray
