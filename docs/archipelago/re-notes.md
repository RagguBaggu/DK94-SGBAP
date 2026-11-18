# Reverse-engineering notes for the Archipelago integration

Target: **Donkey Kong (World) (Rev 1) (SGB Enhanced)**, SHA-1
`397ad2ff25627b83e02c71b54c72bb4deb39e0c0`. Addresses are Rev 1 unless marked
**Rev 0** (the Japan/USA ROM that the [DKGBDisasm](https://github.com/CelestialAmber/DKGBDisasm)
disassembly builds). RAM addresses seem to be shared by both revisions. ROM
addresses are not: Rev 1's code moved by 0x10–0x20 bytes in the areas
checked so far.

Status markers: **confirmed** = seen on Rev 1 (event log, disassembly of the
Rev 1 ROM, or a test). **Rev 0 code** = read from the disassembly, and the routine
was found in Rev 1 by byte pattern. **guess** = not checked.

## Summary for the integration

| Need | How | Status |
|---|---|---|
| stage cleared | `C85C` moves **forward** from a `01/nn` entry (music `30` plays) | confirmed |
| stage number | the parameter of the `01/nn` entry at `C85C` (0..100, play order) | confirmed |
| Pauline items | `C859` bits 0/1/2 set while playing stage `nn`; `C858` counts down from 3 | confirmed |
| death | lives (`DA43`) decrease; `C85C` goes back to the stage's `02` entry | confirmed |
| stage select (not in the game) | on the map, write `C85C` = the stage's `02` position, then `FF8E` = `14`, `FF90` = `00`: the map restarts at that stage | confirmed (test: 1-1 → 2-1) |
| sending a death (DeathLink) | not done yet (see open questions) | – |

## RAM

| Address | Meaning | Status |
|---|---|---|
| `C85C` (16-bit) | position in the scene script (see below) | confirmed |
| `C858` | Pauline items left to collect in this stage; on reaching 0 the game plays sfx `44` and the bonus game follows the clear | confirmed (code) |
| `C859` | Pauline items collected in this stage: bit 0 (tile class `1C`), bit 1 (class `1B`), bit 2 (class `26`). Which bit is the umbrella, hat or purse still needs checking | confirmed (code + log) |
| `C711`–`C712` | stage timer; copied into the stage record on a clear | Rev 0 code |
| `C835` / `C82F` | sound-effect request (the game writes both) | confirmed |
| `C836` | music request (`80` = stop; `30` = stage clear) | confirmed |
| `C838` | bit 7 = paused | upstream notes |
| `DA43` | lives | confirmed |
| `FF8E` | current scene (`hCurrentScene`) | confirmed |
| `FF90` | step within the current scene (`hFunctionTableIndex`); `00` re-initialises the scene | confirmed (stage-select test) |

Scenes seen: `00` boot, `01`/`02` title, `03` file select, `04` arcade intro,
`08` pause menu (CONTINUE / SAVE+END / RECORD), `0F` a single frame when a stage loads,
`10` stage (also the preview), `11` pause, `14` map screen, `15` bonus game (wheel),
`16` bonus game (slots), `1E` cut-scene.

## The scene script (`0C:4F97`)

The game's progress is a single position (`C85C`) in a list of scenes, 2 bytes
each (type, parameter). There are 0x150 entries. Entry types:

| Type | Count | Meaning |
|---|---|---|
| `14/gg` | 26 | map group `gg` begins (`00`–`1A`, not in numeric order); the save checkpoint |
| `02/ll` | 102 | **map screen** before a stage (scene `14`); `ll` = `C8`–`CB` for the arcade stages, otherwise a running index |
| `03/01`, `03/02` | 68 | stage preview (`03/00` at position 0 is the opening cut-scene) |
| `01/nn` | 102 | **play stage `nn`** (`nn` = 0..100 in play order) |
| `04/xx` | 36 | cut-scenes (`04/00` after each group's DK stage, `04/05` at the end of a world, `04/01` after the arcade) |
| `13`, `16` | 1 each | at the very end (credits?) |

One map group (four stages, the last one being the DK fight) looks like:

```
14/gg  02/ll 03/01 01/nn  02/ll 03/02 01/nn+1  02/ll 03/02 01/nn+2  02/ll 01/nn+3  04/00
```

The arcade stages (0-1 to 0-4) are `02/C8 01/00 04/00 …` with no preview.
The Tower (world 9) ends differently: single stages with a cut-scene between each.

Flow (confirmed on Rev 1):

- `02` (map) → `03` (preview) → `01/nn` (play). Briefly the position goes `03` → `01` → `03` → `01` while the stage loads.
- **clear**: `01/nn` → next entry (`02` of the next stage or `04` cut-scene); the game writes the stage record
- **death**: lives decrease (code at `00:0939` or `00:092A`), and the position goes back to the stage's `02` entry, so the player sees the map again
- **SAVE+END** (pause menu): saves the file and resets to the title. **Progress inside the current map group is lost**: the file stores only the group, so continuing starts again at the group's first stage

`upstream`'s stage select (`runtime/cheats.c`) has the `02` position of every
stage (`stage_pos[]`).

## Pickups (bank 7)

Mario's tile collisions go through two tables: tile ID → class (`07:750A`), then
class → handler (`07:75EA`, dispatched at `07:6B44`). Pickup handlers:

| Class | Handler | Effect |
|---|---|---|
| `1C` | `07:6C6A` → `7D96` | Pauline item: `C859` bit 0, sfx `1A` |
| `1B` | `07:6C44` → `7D5E` | Pauline item: `C859` bit 1, sfx `1A` |
| `26` | `07:6DCF` → `7DBE` | Pauline item: `C859` bit 2, sfx `1A` |
| `23` | `07:6D9F` → `7E43` | 1UP heart: +1 life, sfx `37` |
| `24` | `07:6DAF` → `7E55` | 2UP heart: +2 lives, sfx `38` |
| `25` | `07:6DBF` → `7E67` | 3UP heart: +3 lives, sfx `39` |

All three items call `07:7E07`, which decrements `C858` and plays sfx `44` when it reaches 0.

## Routines

| Rev 1 | Rev 0 | What it does | Status |
|---|---|---|---|
| `00:0F39` | `00:0F22` | stage index: counts type `02` (and `15`) entries up to `C85C` | Rev 0 code |
| `00:0F15` | `00:0EFE` | on a clear: writes the timer into the stage record at `A190 + 4·stage` | confirmed (log) |
| `01:4CBE` | `01:4CB3` | saves the file entry (lives, group) with checksum | confirmed (log) |
| `17:74F0` | – | at a map group's start: commits the record block (checksum `BE2A`, mirror `BE2C`, `BFBC`) | confirmed (log) |
| `00:1EC0` | `00:1EA9` | turns save RAM on | Rev 0 code |
| `00:0939` / `00:092A` | – | lose a life on death | confirmed |
| `00:0DF4` | – | add lives (A = count) | confirmed |
| `00:1057` | – | file select / new game: resets lives, clears save RAM ranges | confirmed |
| `01:43F9` | – | leaves the map screen into a stage (sfx `6F`, music stop) | confirmed |

## Save RAM (8 KiB, `A000`–`BFFF`)

Every value is stored **one nibble per byte**: the game writes `v` and then
`swap(v)`, and reads only the low nibble of each. A data byte takes two save
bytes, and the value is `(b0 & 0F) | (b1 & 0F) << 4`. (Confirmed.)

| Range | Contents | Status |
|---|---|---|
| `A000`–`A18F` | committed per-stage records (100 stages × 2 data bytes) | Rev 0 code |
| `A190`–`A31F` | working records for the current run: on a clear, `A190 + 4·stage` = time left | confirmed |
| `B000`–`BDFF` | **not save data**: work buffer for stage tilemaps | confirmed |
| `BE2A`–`BE2B` | checksum of the record block | confirmed |
| `BE2C`–`BFBB` | mirror of the record block | confirmed |
| `BFBC`–`BFBD` | checksum of the mirror | confirmed |
| `BFBE` | selected file slot | Rev 0 code |
| `BFBF` | bit mask of the files in use (`1 << slot`) | confirmed (`01`) |
| `BFC0 + 8·slot` | per file: lives, current map group, highest map group, (4th unknown) | confirmed (lives, group) |
| `BFD8`–`BFD9` | marker, nibbles `2`, `7` | confirmed |
| `BFDA`–`BFDB` | checksum: the sum of the low nibbles of `BFC0`–`BFD9` | confirmed |
| `BFDC`–`BFF9` | mirror of `BFBE`–`BFDB` | Rev 0 code |
| `BFFA`–`BFFF` | signature, compared with ROM `2000`–`2005` | Rev 0 code |

The game's save is too coarse for Archipelago: it keeps only the map group, and
a save mid-group loses that group's clears. The integration should keep its own
save (per seed) outside the game's save RAM.

## Open questions

1. Which `C859` bit is the umbrella, the hat and the purse? (Check one stage
   with `DK94_EVENTLOG_WATCH=C858-C859`.)
2. Is `C859` reset at the start of each stage attempt, or only on a clear?
3. Which stages have Pauline items (the DK stages probably don't; the arcade
   stages 0-2 to 0-4 do)? Can be read from the stage data.
4. DeathLink in: what is the cleanest way to kill Mario? (Find the death routine
   that calls `00:0939`.)
5. Restarting the map scene from the pause menu or mid-stage: is it safe, or must
   the player be on the map first?

## Capturing an event log

The fork has an event logger for this (`runtime/eventlog.c`):

```powershell
$env:DK94_EVENTLOG = "D:\DK94AP\events.txt"
# optional: also log every write to these WRAM ranges
# $env:DK94_EVENTLOG_WATCH = "C858-C859,DA40-DA4F"
.\build-docker-win\dk94.exe
```

Press **F7** in game to drop a numbered marker into the log, for example right
after picking up an item. The `pc` in the log is the start of the recompiled
function or block that made the write, not the exact instruction.
