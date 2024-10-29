// Copyright 2024 Tillitis AB

#include <stdint.h>
#include <stdlib.h>
#include "tkey/assert.h"

void exit(int status)
{
    (void) status;

    assert(1 == 2);

    while (1) {
        __asm__("nop");
    }
}

