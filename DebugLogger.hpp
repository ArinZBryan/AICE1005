#ifndef DEBUG_LOG_HPP
#define DEBUG_LOG_HPP

#include <iostream>

// Enable logging only if DEBUG is defined
#ifdef DEBUG

#define DEBUG_LOG(msg) \
        do { std::cout << msg << std::endl; } while (0)

#else

    // No-op when DEBUG is not defined
#define DEBUG_LOG(msg) \
        do {} while (0)

#endif // DEBUG

#endif // DEBUG_LOG_HPP
