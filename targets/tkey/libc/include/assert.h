#ifndef _ASSERT_H
#define _ASSERT_H

#include "tkey/assert.h"  // IWYU pragma: keep
                          // Including assert macro from tkey-libs

#define static_assert(condition, message) _Static_assert(condition, message)

#endif
