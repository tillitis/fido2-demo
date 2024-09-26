// Copyright 2024 Tillitis AB
//
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.

#include <stddef.h>
#include <stdint.h>

#include "rng.h"
#include "tkey/tk1_mem.h"

static volatile uint32_t *trng_status  = (volatile uint32_t *)TK1_MMIO_TRNG_STATUS;
static volatile uint32_t *trng_entropy = (volatile uint32_t *)TK1_MMIO_TRNG_ENTROPY;

void randombytes(uint8_t *dst, size_t sz)
{
    rng_get_bytes(dst, sz);
}

// Generate random numbers in the simplest way possible. It is not recommended
// for secure applications to use values from the TRNG directly like this.
//
// A random number generator app is available here:
//
// * https://tillitis.se/blog/2023/10/06/how-tkey-random-number-generator-app-works/
// * https://tillitis.se/app/tkey-random-number-generator/
//
// More information on the TKey TRNG can be found here:
//
// * https://tillitis.se/blog/2024/05/27/high-quality-noise-in-a-fpga-how-the-tkey-trng-works/
void rng_get_bytes(uint8_t *dst, size_t sz)
{
    for (int i = 0; i < sz; i++) {
        while ((*trng_status & (1 << TK1_MMIO_TRNG_STATUS_READY_BIT)) == 0) {
        }
        dst[i] = (uint8_t)*trng_entropy;
    }
}

