/* Event log for reverse-engineering the Archipelago integration
 * (DK94_EVENTLOG=file).  Records save-RAM writes, sound/music requests,
 * changes to the scene-script position, scene changes, lives, writes to
 * extra watched ranges (DK94_EVENTLOG_WATCH="C700-C71F,DA40-DA4F") and
 * F7 markers, each with the frame and the code address responsible.
 * Save-RAM writes to the stage tilemap buffer (B000-BDFF) are skipped
 * unless DK94_EVENTLOG_ALLSRAM is set. */
#ifndef EVENTLOG_H
#define EVENTLOG_H

#include <stdbool.h>
#include <stdint.h>

extern bool eventlog_enabled;

void eventlog_init(void);
void eventlog_write(uint16_t addr, uint8_t value);   /* from mem_write */
void eventlog_frame(void);                           /* once per frame */
void eventlog_mark(void);                            /* F7 */

#endif
