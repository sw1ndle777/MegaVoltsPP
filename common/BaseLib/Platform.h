#pragma once

#ifdef _WIN32
    #include <corecrt.h>
    using time32_t = __time32_t;
#else
    #include <cstdint>
    using time32_t = std::int32_t;
#endif
