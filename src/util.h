#ifndef UTIL_H
#define UTIL_H

#include <zephyr/sys/__assert.h>

#define util_assert(expr) __ASSERT(expr, "Assertion failed")

#endif
