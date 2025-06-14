// Copyright 2019 SoloKeys Developers
//
// Licensed under the Apache License, Version 2.0, <LICENSE-APACHE or
// http://apache.org/licenses/LICENSE-2.0> or the MIT license <LICENSE-MIT or
// http://opensource.org/licenses/MIT>, at your option. This file may not be
// copied, modified, or distributed except according to those terms.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "device.h"

#include APP_CONFIG
#include "flash.h"
#include "rng.h"
#include "device.h"
#include "fifo.h"
#include "log.h"
#include "ctaphid.h"
#include "ctap.h"
#include "memory_layout.h"
#include "init.h"
#include "tkey/assert.h"
#include "tkey/io.h"
#include "tkey/led.h"
#include "tkey/tk1_mem.h"
#include "timer.h"
#include "frame.h"

static volatile uint32_t *timer = (volatile uint32_t *)TK1_MMIO_TIMER_TIMER;
static volatile uint32_t *touch = (volatile uint32_t *)TK1_MMIO_TOUCH_STATUS;

#define SOLO_FLAG_LOCKED                    0x2

#define HID_PACKET_SIZE                 64

uint32_t __device_status = 0;
static bool _up_disabled = false;

void device_disable_up(bool disable)
{
    _up_disabled = disable;
}

uint32_t millis(void)
{
    uint32_t timer_val = *timer;
    if (timer_val == 0) {
        assert(1 == 2);
    }
    return TIMER_MAX - timer_val;
}

void device_set_status(uint32_t status)
{
    if (status != CTAPHID_STATUS_IDLE && __device_status != status)
    {
        ctaphid_update_status(status);
    }
    __device_status = status;
}

void delay(uint32_t ms)
{
    uint32_t time = millis();
    while ((millis() - time) < ms)
        ;
}

void device_reboot(void)
{
    printf1(TAG_ERR,"Tried to reboot TKey");
    assert(1 == 2);
}

int solo_is_locked(){
    uint64_t device_settings = ((flash_attestation_page *)ATTESTATION_PAGE_ADDR)->device_settings;
    uint32_t tag = (uint32_t)(device_settings >> 32ull);
    return tag == ATTESTATION_CONFIGURED_TAG && (device_settings & SOLO_FLAG_LOCKED) != 0;
}

// Locks solo flash from debugging.  Locks on next reboot.
// This should be removed in next Solo release.
void solo_lock_if_not_already() {
    uint8_t buf[2048];

    memmove(buf, (uint8_t*)ATTESTATION_PAGE_ADDR, 2048);

    ((flash_attestation_page *)buf)->device_settings |= SOLO_FLAG_LOCKED;

    flash_erase_page(ATTESTATION_PAGE);

    flash_write(ATTESTATION_PAGE_ADDR, buf, 2048);
}

/** device_migrate
 * Depending on version of device, migrates:
 * * Moves attestation certificate to data segment. 
 * * Creates locked variable and stores in data segment.
 * 
 * Once in place, this allows all devices to accept same firmware,
 * rather than using "hacker" and "secure" builds.
*/
static void device_migrate(){
    extern const uint16_t attestation_solo_cert_der_size;
    extern const uint16_t attestation_hacker_cert_der_size;

    extern uint8_t attestation_solo_cert_der[];
    extern uint8_t attestation_hacker_cert_der[];

    flash_attestation_page attestation;
    flash_read(ATTESTATION_PAGE_ADDR, (uint8_t*)&attestation, sizeof(attestation));

    uint64_t device_settings = attestation.device_settings;
    uint32_t configure_tag = (uint32_t)(device_settings >> 32);

    if (configure_tag != ATTESTATION_CONFIGURED_TAG)
    {
        printf1(TAG_RED,"Migrating certificate and lock information to data segment.\r\n");

        device_settings = ATTESTATION_CONFIGURED_TAG;
        device_settings <<= 32;

        uint8_t tmp_attestation_key[32];

        memmove(tmp_attestation_key, attestation.attestation_key, 32);
        memset(&attestation, 0xff, sizeof(attestation));
        memmove(&attestation.attestation_key, tmp_attestation_key, 32);
        flash_erase_page(ATTESTATION_PAGE);

        memmove(&attestation.attestation_cert_size, &attestation_solo_cert_der_size, 8);
        memmove(&attestation.attestation_cert, &attestation_solo_cert_der, attestation_solo_cert_der_size);
        memmove(&attestation.device_settings, &device_settings, sizeof(device_settings));

        flash_write(ATTESTATION_PAGE_ADDR, (uint8_t *)&attestation, sizeof(attestation));
    }
}

void device_init()
{
    hw_init();
    device_migrate();
    // usbhid_init();
    ctaphid_init();
    ctap_init();
}

int device_is_nfc(void)
{
    return 0;
}

void usbhid_init(void)
{
}

int usbhid_recv(uint8_t * msg)
{
    if (fifo_hidmsg_size())
    {
        fifo_hidmsg_take(msg);
        printf1(TAG_DUMP2,">> ");
        dump_hex1(TAG_DUMP2,msg, HID_PACKET_SIZE);
        return HID_PACKET_SIZE;
    }
    return 0;
}

void usbhid_send(uint8_t * msg)
{
    printf1(TAG_DUMP2,"<< ");
    dump_hex1(TAG_DUMP2, msg, HID_PACKET_SIZE);
    write(IO_FIDO, msg, HID_PACKET_SIZE);
}

void usbhid_close(void)
{
    // TKey USB cannot be closed
    assert(1 == 2);
}

void device_wink(void)
{
    printf1(TAG_ERR,"WINK not implemented\n");
    assert(1 == 2);
}

static int authenticator_is_backup_initialized(void)
{
    uint8_t header[16];
    flash_read(flash_addr(STATE2_PAGE), header, sizeof(header));
    AuthenticatorState * state = (AuthenticatorState*)header;
    return state->is_initialized == INITIALIZED_MARKER;
}

int authenticator_read_state(AuthenticatorState * a)
{
    flash_read(flash_addr(STATE1_PAGE), (uint8_t*)a, sizeof(AuthenticatorState));

    if (a->is_initialized != INITIALIZED_MARKER){

        if (authenticator_is_backup_initialized()){
            printf1(TAG_ERR,"Warning: memory corruption detected.  restoring from backup..\n");
            flash_read(flash_addr(STATE2_PAGE), (uint8_t*)a, sizeof(AuthenticatorState));
            authenticator_write_state(a);
            return 1;
        }

        return 0;
    }

    return 1;
}


void authenticator_write_state(AuthenticatorState * a)
{
    flash_erase_page(STATE1_PAGE);
    flash_write(flash_addr(STATE1_PAGE), (uint8_t*)a, sizeof(AuthenticatorState));

    flash_erase_page(STATE2_PAGE);
    flash_write(flash_addr(STATE2_PAGE), (uint8_t*)a, sizeof(AuthenticatorState));
}

uint32_t ctap_atomic_count(uint32_t amount)
{
    int offset = 0;
    uint32_t erases = 0; // *(uint32_t *)flash_addr(COUNTER2_PAGE);
    static uint32_t sc = 0;

    flash_read(flash_addr(COUNTER2_PAGE), (uint8_t*)&erases, sizeof(erases));
    printf2(TAG_COUNT, "read erases: %lu\r\n", erases);

    if (erases == 0xffffffff)
    {
        erases = 1;
        flash_erase_page(COUNTER2_PAGE);
        flash_write(flash_addr(COUNTER2_PAGE), (uint8_t*)&erases, 4);
        printf2(TAG_COUNT, "wrote erases: %lu\r\n", erases);
    }

    uint32_t lastc = 0;

    if (amount == 0)
    {
        // Use a random count [1-16].
        uint8_t rng[1];
        ctap_generate_rng(rng, 1);
        amount = (rng[0] & 0x0f) + 1;
    }

    for (offset = 0; offset < PAGE_SIZE/4; offset += 2) // wear-level the flash
    {
        uint32_t val;
        flash_read(flash_addr(COUNTER1_PAGE) + offset * sizeof(val), (uint8_t*)&val, sizeof(val));
        printf2(TAG_COUNT, "read count %lu from offset %lu\r\n", val, offset);

        if (val != 0xffffffff)
        {
            if (val < lastc)
            {
                printf2(TAG_ERR,"Error, count went down!\r\n");
            }
            lastc = val;
        }
        else
        {
            break;
        }
    }

    if (!lastc) // Happens on initialization as well.
    {
        printf2(TAG_ERR,"warning, power interrupted during previous count.  Restoring. lastc==%lu, erases=%lu, offset=%d\r\n", lastc,erases,offset);
        // there are 32 counts per page
        lastc =  erases * 256 + 1;
        flash_erase_page(COUNTER1_PAGE);
        flash_write(flash_addr(COUNTER1_PAGE), (uint8_t*)&lastc, 4);
        printf2(TAG_COUNT, "wrote lastc: %lu\r\n", lastc);

        erases++;
        flash_erase_page(COUNTER2_PAGE);
        flash_write(flash_addr(COUNTER2_PAGE), (uint8_t*)&erases, 4);
        printf2(TAG_COUNT, "wrote erases: %lu\r\n", erases);

        printf2(TAG_COUNT, "lastc is 0, new lastc: %lu, new erase: %lu\r\n", lastc, erases);
        return lastc;
    }

    if (amount > 256){
        lastc = amount;
    } else {
        lastc += amount;
    }

    if (lastc/256 > erases)
    {
        printf2(TAG_ERR,"warning, power interrupted, erases mark, restoring. lastc==%lu, erases=%lu\r\n", lastc,erases);
        erases = lastc/256;
        flash_erase_page(COUNTER2_PAGE);
        flash_write(flash_addr(COUNTER2_PAGE), (uint8_t*)&erases, 4);
        printf2(TAG_COUNT, "wrote erases: %lu\r\n", erases);
    }

    if (offset == PAGE_SIZE/4)
    {
        if (lastc/256 > erases)
        {
            printf2(TAG_ERR,"warning, power interrupted, erases mark, restoring lastc==%lu, erases=%lu\r\n", lastc,erases);
        }
        erases = lastc/256 + 1;
        flash_erase_page(COUNTER2_PAGE);
        flash_write(flash_addr(COUNTER2_PAGE), (uint8_t*)&erases, 4);
        printf2(TAG_COUNT, "wrote erases: %lu\r\n", erases);

        flash_erase_page(COUNTER1_PAGE);
        printf2(TAG_COUNT, "erased lastc\r\n");
        offset = 0;
    }


    flash_write(flash_addr(COUNTER1_PAGE) + offset * 4, (uint8_t*)&lastc, 4);
    printf2(TAG_COUNT, "wrote count %lu to offset %lu\r\n", lastc, offset);

    if (lastc == sc)
    {
        printf1(TAG_RED,"no count detected:  lastc==%lu, erases=%lu, offset=%d\r\n", lastc,erases,offset);
        while(1)
            ;
    }

    sc = lastc;

    printf2(TAG_COUNT, "returning count: %lu\r\n", lastc);
    return lastc;
}

int ctap_user_presence_test(uint32_t up_delay)
{
    if (_up_disabled)
    {
        return 2;
    }

    uint32_t start_time = millis();
    uint32_t time;
    bool led_on;

    *touch = 0;
    do {
        if (*touch & (1 << TK1_MMIO_TOUCH_STATUS_EVENT_BIT)) {
            led_set(LED_GREEN | LED_RED);
            return 1;
        }

        time = millis();
        led_on = ((time - start_time) / 100 % 2);
        led_set(led_on ? LED_GREEN : LED_BLACK);
    } while ((time - start_time) < up_delay);

    led_set(LED_BLACK);

    return 0;
}

int ctap_generate_rng(uint8_t * dst, size_t num)
{
    rng_get_bytes(dst, num);
    return 1;
}


void ctap_reset_rk(void)
{
    int i;
    printf1(TAG_GREEN, "resetting RK \r\n");
    for(i = 0; i < RK_NUM_PAGES; i++)
    {
        flash_erase_page(RK_START_PAGE + i);
    }
}

uint32_t ctap_rk_size(void)
{
    return RK_NUM_PAGES * (PAGE_SIZE / sizeof(CTAP_residentKey));
}

void ctap_store_rk(int index,CTAP_residentKey * rk)
{
    ctap_overwrite_rk(index, rk);
}

void ctap_delete_rk(int index)
{
    CTAP_residentKey rk;
    memset(&rk, 0xff, sizeof(CTAP_residentKey));
    ctap_overwrite_rk(index, &rk);
}

void ctap_load_rk(int index,CTAP_residentKey * rk)
{
    int byte_offset_into_page = (sizeof(CTAP_residentKey) * (index % (PAGE_SIZE/sizeof(CTAP_residentKey))));
    int page_offset = (index)/(PAGE_SIZE/sizeof(CTAP_residentKey));

    uint32_t addr = flash_addr(page_offset + RK_START_PAGE) + byte_offset_into_page;

    printf1(TAG_GREEN, "reading RK %d @ %04x\r\n", index, addr);
    if (page_offset < RK_NUM_PAGES)
    {
        flash_read(addr, (uint8_t*)rk, sizeof(CTAP_residentKey));
    }
    else
    {
        printf2(TAG_ERR,"Out of bounds reading index %d for rk\n", index);
    }
}

void ctap_overwrite_rk(int index,CTAP_residentKey * rk)
{
    uint8_t tmppage[PAGE_SIZE];

    int byte_offset_into_page = (sizeof(CTAP_residentKey) * (index % (PAGE_SIZE/sizeof(CTAP_residentKey))));
    int page_offset = (index)/(PAGE_SIZE/sizeof(CTAP_residentKey));

    printf1(TAG_GREEN, "overwriting RK %d @ page %d @ addr 0x%08x-0x%08x\r\n", 
        index, RK_START_PAGE + page_offset, 
        flash_addr(RK_START_PAGE + page_offset) + byte_offset_into_page, 
        flash_addr(RK_START_PAGE + page_offset) + byte_offset_into_page + sizeof(CTAP_residentKey) 
        );
    if (page_offset < RK_NUM_PAGES)
    {
        flash_read(flash_addr(RK_START_PAGE + page_offset), tmppage, sizeof(tmppage));

        memmove(tmppage + byte_offset_into_page, rk, sizeof(CTAP_residentKey));
        flash_erase_page(RK_START_PAGE + page_offset);
        flash_write(flash_addr(RK_START_PAGE + page_offset), tmppage, PAGE_SIZE);
    }
    else
    {
        printf2(TAG_ERR,"Out of bounds reading index %d for rk\n", index);
    }
    printf1(TAG_GREEN, "4\r\n");
}

void boot_solo_bootloader(void)
{
    printf1(TAG_ERR,"Bootloader not implemented\n");
    assert(1 == 2);
}

void device_read_aaguid(uint8_t * dst){
    uint8_t * aaguid = (uint8_t *)"\x88\x76\x63\x1b\xd4\xa0\x42\x7f\x57\x73\x0e\xc7\x1c\x9e\x02\x79";
    memmove(dst, aaguid, 16);
    dump_hex1(TAG_GREEN,dst, 16);
}

