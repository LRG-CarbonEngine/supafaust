# Supafaust

This is Supafaust, the libretro port of Mednafen's experimental snes_faust
module, a faster eventual replacement for its old bsnes-based snes module
(which forms the basis of the [Beetle
bSNES](https://www.github.com/libretro/beetle-bsnes-libretro) core). It's
particularly suitable for low-end devices, such as multicore ARM Cortex
A7,A9,A15,A53 Linux platforms.

If you run into any problems with it, please don't bother upstream. Go to the
[libretro forums](https://forums.libretro.com) and we'll try to help you as
best we can.

Many thanks to the author(s) for working on this emulator :)

Minimum recommended CPU is a dual-core CPU, running at 900MHz for most SNES
games, and at 1200MHz for SuperFX, SA-1, and CX4 games.

Special chips supported: DSP-1, DSP-2, CX4, SuperFX, SA-1, S-DD1, MSU1

MSU1 support may have performance issues with runahead and/or state rewinding.

SPC playback is supported, to an extent.

Save states are not currently compatible across different architectures
(e.g. 32-bit ARM vs 64-bit ARM).

Save states are not currently sanitized properly on load, so don't load
save states from untrusted sources.

Core option changes take effect upon game load.

Setting "supafaust_frame_begin_vblank" to "disabled" will improve performance
but also increase input lag on most games.

Setting "supafaust_thread_affinity_emu" to "0x3" and
"supafaust_thread_affinity_ppu" to "0xc" may give better performance on a
quad-core CPU.

Setting "supafaust_renderer" to "st", to disable multithreaded PPU rendering,
may improve performance on systems with only a single CPU with a single core.
