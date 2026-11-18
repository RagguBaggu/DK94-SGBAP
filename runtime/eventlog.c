/* Event log for reverse-engineering the Archipelago integration; see
 * eventlog.h.  Addresses are for Donkey Kong (World) (Rev 1). */
#include <stdio.h>
#include <stdlib.h>
#include "gb.h"
#include "cheats.h"
#include "eventlog.h"

#define SCRIPT_BANK 0x0C
#define SCRIPT_ADDR 0x4F97     /* scene script: 2 bytes per entry (type, param) */
#define PROGRESS    0xC85C     /* 16-bit position in the scene script */
#define SFX_REQ     0xC835
#define MUSIC_REQ   0xC836
#define LIVES       0xDA43
#define SCENE_HRAM  0x0E       /* hCurrentScene = FF8E */

bool eventlog_enabled;
static FILE *out;

#define MAX_WATCH 16
static struct { uint16_t lo, hi; } watch[MAX_WATCH];
static int n_watch;

void eventlog_init(void)
{
    const char *path = getenv("DK94_EVENTLOG");
    if (!path)
        return;
    out = fopen(path, "w");
    if (!out) {
        fprintf(stderr, "eventlog: cannot open %s\n", path);
        return;
    }
    setvbuf(out, NULL, _IOLBF, 0);
    eventlog_enabled = true;
    for (const char *p = getenv("DK94_EVENTLOG_WATCH"); p && *p && n_watch < MAX_WATCH;) {
        char *end;
        unsigned long lo = strtoul(p, &end, 16), hi = lo;
        if (end == p)
            break;
        if (*end == '-')
            hi = strtoul(end + 1, &end, 16);
        watch[n_watch].lo = (uint16_t)lo;
        watch[n_watch].hi = (uint16_t)hi;
        n_watch++;
        p = *end == ',' ? end + 1 : end;
    }
    fprintf(out, "# DK94 event log. pc = address of the instruction after the write\n");
}

/* "BB:AAAA" of the code currently running (R.pc is already past the write) */
static const char *where(void)
{
    static char buf[16];
    uint16_t pc = R.pc;
    snprintf(buf, sizeof buf, "%02X:%04X", pc >= 0x4000 && pc < 0x8000 ? gb.rom_bank : 0, pc);
    return buf;
}

static uint8_t script_byte(unsigned pos, unsigned which)
{
    size_t off = (size_t)SCRIPT_BANK * 0x4000 + (SCRIPT_ADDR - 0x4000) + pos * 2 + which;
    return off < (size_t)gb.rom_banks * 0x4000 ? gb.rom[off] : 0xFF;
}

/* stage number (0-based, play order) of the stage at or before pos */
static int stage_at(unsigned pos)
{
    int n = -1;
    for (unsigned i = 0; i <= pos; i++)
        if (script_byte(i, 0) == 0x02)
            n++;
    return n;
}

void eventlog_write(uint16_t a, uint8_t v)
{
    if (a >= 0xA000 && a < 0xC000) {
        /* B000-BDFF is the game's work buffer for stage tilemaps, not save
         * data: skip it unless asked for */
        static int all_sram = -1;
        if (all_sram < 0)
            all_sram = getenv("DK94_EVENTLOG_ALLSRAM") != NULL;
        if (!all_sram && a >= 0xB000 && a < 0xBE00)
            return;
        if (gb.sram_enabled)
            fprintf(out, "%llu sram %04X=%02X  pc %s\n",
                    (unsigned long long)gb.frame_count, a, v, where());
        return;
    }
    if (a == SFX_REQ && v) {
        fprintf(out, "%llu sfx %02X  pc %s\n", (unsigned long long)gb.frame_count, v, where());
        return;
    }
    if (a == MUSIC_REQ && v) {
        fprintf(out, "%llu music %02X  pc %s\n", (unsigned long long)gb.frame_count, v, where());
        return;
    }
    if (a == LIVES && v != gb.wram[LIVES - 0xC000]) {
        fprintf(out, "%llu lives %02X -> %02X  pc %s\n", (unsigned long long)gb.frame_count,
                gb.wram[LIVES - 0xC000], v, where());
        return;
    }
    for (int i = 0; i < n_watch; i++) {
        if (a >= watch[i].lo && a <= watch[i].hi && a >= 0xC000 && a < 0xE000 &&
            gb.wram[a - 0xC000] != v) {
            fprintf(out, "%llu w %04X %02X -> %02X  pc %s\n", (unsigned long long)gb.frame_count,
                    a, gb.wram[a - 0xC000], v, where());
            return;
        }
    }
}

void eventlog_frame(void)
{
    static unsigned prev_pos = 0xFFFF, prev_scene = 0xFFFF;
    if (!eventlog_enabled)
        return;
    unsigned scene = gb.hram[SCENE_HRAM];
    if (scene != prev_scene) {
        fprintf(out, "%llu scene %02X\n", (unsigned long long)gb.frame_count, scene);
        prev_scene = scene;
    }
    unsigned pos = gb.wram[PROGRESS - 0xC000] | gb.wram[PROGRESS + 1 - 0xC000] << 8;
    if (pos != prev_pos) {
        uint8_t t = script_byte(pos, 0), p = script_byte(pos, 1);
        char label[32] = "";
        int st = stage_at(pos);
        if (st >= 0 && st < stage_count())
            stage_label(st, label, sizeof label);
        fprintf(out, "%llu progress %03X -> %03X  entry %02X/%02X  stage %d (%s)\n",
                (unsigned long long)gb.frame_count, prev_pos & 0xFFF, pos, t, p, st, label);
        prev_pos = pos;
    }
}

void eventlog_mark(void)
{
    static int n;
    if (eventlog_enabled)
        fprintf(out, "%llu MARK %d ----------------------------------------\n",
                (unsigned long long)gb.frame_count, ++n);
}
