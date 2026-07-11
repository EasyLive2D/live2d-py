#pragma once
// Debug macros for v2cpp rendering
// Define V2CPP_DEBUG to enable debug output
#ifdef V2CPP_DEBUG
#include <cstdio>
#define DBG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...) ((void)0)
#endif
