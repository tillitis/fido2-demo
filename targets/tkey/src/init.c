// Copyright 2019 SoloKeys Developers
//
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.
#include <stdint.h>

#include "init.h"
#include APP_CONFIG

void hw_init()
{
    init_millisecond_timer();

#if DEBUG_LEVEL > 0
    init_debug_uart();
#endif
    init_rng();
}

void init_debug_uart(void)
{
}

void init_millisecond_timer()
{
}

void init_rng(void)
{
}

