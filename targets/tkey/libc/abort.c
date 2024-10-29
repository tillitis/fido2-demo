// Copyright 2024 Tillitis AB

#include <stdlib.h>
#include "tkey/assert.h"

void abort(void)
{
    assert(1 == 2);

    while (1) {
        __asm__("nop");
    }
}

