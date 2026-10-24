/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 M-Vave-OP1 contributors */
/* M-Vave-OP1, MENU > USB DISK's screen (firmware/src/usbdisk.c dsk_title, dsk_status) on ui_test.c's harness: the
 * title, the files the disk has with their projects' names, a status; every line inside the screen and none cut short
 * (its width against the font's), in the colour palette and the grey one. With a directory: the screens as PPM.
 * Built and run by tests/run_tests.sh. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#define USBDISK_SCREEN_ONLY 1
#include "../firmware/src/usbdisk.c"

static int bad;
static int ck(const char *what, int ok)
{
    printf("vdisk_screen: %-90s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok)
        bad++;
    return ok;
}
static void ppm(const char *dir, const char *name)
{
    char path[512];
    uint32_t i;
    FILE *f;
    snprintf(path, sizeof path, "%s/%s.ppm", dir, name);
    f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "P6\n240 240\n255\n");
    for (i = 0; i < 240u * 240u; i++) {
        uint16_t c = swap16(host_screen[i]);
        uint8_t b[3] = {(uint8_t)((c >> 11) * 255u / 31u), (uint8_t)(((c >> 5) & 63u) * 255u / 63u),
                        (uint8_t)((c & 31u) * 255u / 31u)};
        fwrite(b, 1, 3, f);
    }
    fclose(f);
}
static uint32_t lit_rows(uint32_t y0, uint32_t y1)   /* rows of y0..y1-1 with anything but the background */
{
    uint32_t x, y, n = 0, bg = host_screen[0];
    for (y = y0; y < y1; y++)
        for (x = 0; x < 240u; x++)
            if (host_screen[y * 240u + x] != bg) {
                n++;
                break;
            }
    return n;
}

int main(int argc, char **argv)
{
    static const char *const LINES[] = {"CONNECT A PHONE OR A COMPUTER", "CURRENT.FM1  LONGESTNAME1",
                                        "ANY BUTTON: BACK", "WAITING FOR USB", "EJECTED: UNPLUG"};
    const char *dir = argc > 1 ? argv[1] : 0;
    uint32_t k, ok = 1, pal;
    for (k = 0; k < sizeof LINES / sizeof LINES[0]; k++)
        ok &= text_w(k >= 3u ? &AF_M : &AF_S, LINES[k]) <= 232;
    ck("every line of it fits the screen's width (the longest name: 12 letters)", ok);
    for (pal = 0; pal < 2u; pal++) {
        ui_power_on();
        if (pal)
            palette_set(UI_GREY_INDEX);
        memset(&dsk, 0, sizeof dsk);
        for (k = 0; k < DISK_NF; k++)
            if (k != 2u && k != 4u) {           /* CURRENT, A, C */
                dsk.h[k].n = 3;
                str_cpy(dsk.h[k].name, k == 0u ? "LONGESTNAME1" : k == 1u ? "FIRST" : "", sizeof dsk.h[k].name);
            }
        dsk_title();
        dsk_status("CONNECTED");
        ok = lit_rows(0, 30) > 8u && lit_rows(76, 76 + 3 * 18) > 20u && lit_rows(76 + 3 * 18, 172) == 0u &&
             lit_rows(180, 200) > 8u && lit_rows(206, 224) > 6u;
        ck(pal ? "  GREY: the title, three files, the status, the hint; nothing where no file is" :
                 "  the title, three files (CURRENT, PROJ_A, PROJ_C), the status, the hint; nothing below the files", ok);
        if (dir)
            ppm(dir, pal ? "usb-disk-grey" : "usb-disk");
    }
    printf(bad ? "vdisk_screen: %d FAILED\n" : "vdisk_screen: all ok\n", bad);
    return bad != 0;
}
