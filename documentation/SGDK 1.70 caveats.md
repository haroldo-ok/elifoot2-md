
# SGDK 1.70 Caveats for Code Generation

## Toolchain & Types

**`int` is 16-bit on m68k.** The SGDK GCC target is `-m68000`, where `int` = 16 bits, `long` = 32 bits, `short` = 16 bits. This is the single most important fact. Consequences:
- Never use `int` where you need >32767. Use `long`.
- Struct layout differs from x86. `{ char* p; int n; long x; }` has `long x` at offset 6, which is misaligned on m68k. GCC adds padding automatically, but your mental model of struct sizes will be wrong if you think `int` is 32 bits.
- `sizeof(int)` = 2, `sizeof(long)` = 4, `sizeof(void*)` = 4.

**SGDK's `string.h` uses non-standard return types.** `strlen` returns `u16`, `strcmp` returns `s16`, `strncpy`/`strncmp` take `u16` length. If you forward-declare these functions with standard C types (`size_t`, `int`), you get "conflicting types" errors at compile time. Either don't declare them at all (SGDK's `genesis.h` covers them), or guard your declarations with `#ifndef SGDK_GCC`.

**`bool` is `unsigned short` (`u16`) in SGDK's `types.h`.** Not `uint8_t`. If you write `bool main(bool)` and SGDK's `sys.c` declares it the same way, LTO will catch a mismatch if your `bool` resolves to a different width. Use `u16 main(u16 hardReset)` to be explicit.

**`#pragma GCC diagnostic ignored "-Wdangling-else"` does not exist** in the older m68k GCC that ships with SGDK 1.70. The pragma itself emits a warning. Stick to pragmas the toolchain actually supports: `-Wunused-value`, `-Wunused-variable`, `-Wchar-subscripts`, `-Wparentheses`, `-Woverflow`, `-Wdiscarded-qualifiers`, `-Wmisleading-indentation`, `-Wincompatible-pointer-types`.

---

## `libmd.a` — What It Does and Doesn't Provide

The linker will fail with "undefined reference" for functions you assume are in the C runtime. SGDK's `libmd.a` does **not** provide:
- `memmove` — you must implement it
- `atoi` — you must implement it  
- `strncmp` — you must implement it

It **does** provide: `memcpy`, `memset`, `strlen`, `strcmp`, `strcpy`, `strncpy`, `strcat`, `strncat`, `strchr`, `sprintf`, `vsprintf`, `snprintf`, `atoi`... *wait, actually check the linker output.* The set varies. Trust the linker errors, not assumptions. When you add a function to your compat layer, do not guard it with `#ifndef SGDK_GCC` — it still needs to compile and link for the SGDK build.

---

## `SYS_doVBlankProcess()` — The Most Dangerous Function

This function **blocks until the next vertical blank interrupt** (~16ms at 60fps). It is not a yield or a hint — it is a full 16ms stall.

**Never call it inside computation loops.** sorting, pathfinding — none of these should call it. Every spurious call adds 16ms of freeze visible to the player. 120 loop iterations × 16ms = 2 seconds of frozen screen.

Only call it from your main vsync function, once per frame, in the input-polling loop.

---

## `JOY_init()` Must Be Called Before `JOY_readJoypad()`

`JOY_readJoypad()` returns 0 until `JOY_init()` has initialised the joypad hardware. This is not obvious from the API. If your title screen loop checks `JOY_readJoypad()` without a prior `JOY_init()`, it will never see input and appear completely frozen. Call `JOY_init()` as the very first thing in your init sequence.

---

## VDP Text

**`VDP_drawText()` writes to the WINDOW plane**, not BG_A or BG_B. You must call `VDP_clearPlane(WINDOW, TRUE)` during init or old text will persist. If you clear BG_A and BG_B but not WINDOW, garbage from a previous state can remain visible.

**`VDP_setTextPalette(n)`** selects which hardware palette (0–3) the next `VDP_drawText` call uses. Call it before each `VDP_drawText` if you want coloured text. The default is palette 0.

**Text is 40×28 tiles at 320×224** (8×8px each). Row 27 is the last visible row. Content at row 28+ is off-screen. Keep your UI layout within rows 0–27 and columns 0–39.

---

## LTO Type Mismatches

SGDK builds with `-flto`. This means the linker runs a type-consistency check across all compilation units. If you declare a function in one file with return type `int` and define it in another with return type `Boolean`, LTO will emit a warning (and may misoptimize). These appear as "type of 'X' does not match original declaration" in the link step, not the compile step.

The fix is always to make declarations match definitions exactly. Common culprits: functions declared `int` in a header but defined `Boolean` (or vice versa) in the implementation, and `main()` whose signature must exactly match what `sys.c` expects.

---

## SRAM

`SRAM_enable()` must be called before any `SRAM_readByte`/`SRAM_writeByte`, and `SRAM_disable()` after. On real hardware, the SRAM chip-select is controlled by these calls. Forgetting them means reads return garbage and writes are lost silently in emulators but fail on hardware.

SGDK 1.70 does not provide `SRAM_readBuffer`/`SRAM_writeBuffer` — implement them yourself as byte-loop wrappers around `SRAM_readByte`/`SRAM_writeByte`.

---

## Build System

SGDK 1.70 uses `makefile.gen`. The standard invocation is:

```bash
make -f /path/to/sgdk170/makefile.gen
```

It defines `-DSGDK_GCC` automatically. Use this flag in your own code to guard declarations that conflict with SGDK's headers:

```c
#ifndef SGDK_GCC
extern size_t strlen(const char* s);  /* x86 test builds only */
#endif
```

The makefile compiles all `.c` files in `src/` automatically — no explicit file list needed. Output is `out/rom.bin`.

---

## Testing Without Hardware

Use a mock `genesis.h` that stubs all SGDK functions as empty inlines and defines all SGDK types. This lets you run `gcc -fsyntax-only` on all source files on your x86 host to catch type errors before flashing. The mock must use SGDK's actual type signatures (especially `u16 strlen`, `s16 strcmp`) or you'll get false passes on the host that become real errors on the target.

**On `-fno-builtin`** — the mechanism here is subtle and worth clarifying: `-fno-builtin` actually *prevents* GCC from substituting calls to standard functions with builtins. So `memset()` calls stay as real calls into SGDK's `memset`. The problem with `__builtin_memset` is that you're *explicitly* invoking the builtin, bypassing that protection entirely, and the builtin has a different signature (returns `void*`). So this is a "don't use explicit builtins" rule, not a `-fno-builtin` interaction per se.

**On rescomp paths** — one thing missing: rescomp generates a header (typically `res/resources.h`) that you `#include` to get the extern declarations for your assets. The generated symbol names follow the pattern `name` from the `.res` file, so `TILESET myTiles "..."` gives you `extern TileSet myTiles;`. If the name in the `.res` file doesn't match what the code references, you get an undefined reference at link time, not a compile error — which can be confusing.

