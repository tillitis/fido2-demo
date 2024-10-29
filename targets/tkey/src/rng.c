// Copyright 2019 SoloKeys Developers
//
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.

#include <stddef.h>
#include <stdint.h>
#include "rng.h"

void randombytes(uint8_t *dst, size_t sz)
{
    rng_get_bytes(dst, sz);
}

void rng_get_bytes(uint8_t *dst, size_t sz)
{
}
