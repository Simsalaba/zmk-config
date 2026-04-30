# Corne Keyboard Layout Guide

> **Board:** nice!nano v2 · **Shield:** Corne (42 keys) · **Displays:** nice!view  
> **OS Layout:** Swedish (SE) — keycodes map to Swedish characters at OS level

---

## 🔌 Flashing Firmware

### Building

```bash
# Recommended: build via GitHub Actions (fast)
./build.sh

# Or build locally with Docker (slow on Apple Silicon)
./build.sh local

# Build only one half locally
./build.sh local left

# Clean build cache
./build.sh clean
```

The default `./build.sh` pushes your current branch, triggers GitHub Actions,
waits for the build, and downloads the `.uf2` files to `firmware/`.

### Flashing

1. Download both `.uf2` files from the latest GitHub Actions build
2. Connect one half via USB
3. **Double-tap the reset button** → a USB drive called `NICENANO` appears
4. Drag the matching `.uf2` file onto the drive → it auto-reboots
5. Repeat for the other half

> Flash each half separately. Left half = `corne_left`, right half = `corne_right`.

---

## 🗺️ Layer Overview

| # | Name       | Purpose                    | How to activate                           |
|---|------------|----------------------------|-------------------------------------------|
| 0 | **Main**   | Letters, home row mods     | Default layer                             |
| 1 | **Numbers**| Number pad + numlock       | Hold left thumb middle key                |
| 2 | **Symbols**| Special characters         | Hold right thumb middle key               |
| 3 | **Navi**   | Arrow keys, page nav       | Hold E (tap-hold)                         |
| 4 | **Adjust** | Bluetooth, media, brightness | **Tri-layer:** hold NUM + CHARS together |
| 5 | **Func**   | F1–F12 keys                | Hold B (tap-hold)                         |
| 6 | **Tiling** | Window tiling (RC+RA combos) | Hold R (tap-hold)                       |
| 7 | **Game**   | Gaming mode (no left HRMs) | Combo: press U + P together               |

---

## ⌨️ Layer 0: Main

```
┌──────┬──────┬──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┬──────┬──────┐
│ ESC  │  Q   │  W   │E/NAV │R/WIN │  T   │  │  Y   │  U   │  I   │  O   │  P   │ DEL  │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│CT/Tab│  A   │S/Alt │D/Shft│F/Ctrl│  G   │  │  H   │J/Ctrl│K/Shft│L/Alt │  Ö   │  Å   │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ SHFT │  Z   │  X   │  C   │  V   │B/FUNC│  │  N   │  M   │  .   │  ,   │      │  Ä   │
└──────┴──────┴──┬───┴──┬───┴──┬───┴──┬───┘  └───┬──┴──┬───┴──┬───┴──┬───┴──────┴──────┘
                 │ GUI  │ NUM  │ SPC  │           │ ENT │CHARS │ BKSP │
                 └──────┴──────┴──────┘           └─────┴──────┴──────┘
```

### Home Row Mods (S, D, F / J, K, L)

The middle three keys on each home row side double as modifiers when **held**:

| Key | Tap   | Hold    |
|-----|-------|---------|
| S   | s     | Left Alt|
| D   | d     | Left Shift|
| F   | f     | Left Ctrl|
| J   | j     | Right Ctrl|
| K   | k     | Right Shift|
| L   | l     | Right Alt|

**How they work:** These use urob's "timeless" homerow mod approach with `balanced` flavor — the modifier activates when you hold the key AND press+release another key on the **opposite** hand. You don't need to wait for any timer. During normal typing, `require-prior-idle` ensures no delays or misfires.

**Timing tuning (based on [urob's timeless HRMs](https://github.com/vqc/zmk-config-hmr-example)):**
- `flavor`: balanced — mod triggers on cross-hand press+release, not on timer expiry
- `hold-trigger-on-release`: allows combining multiple mods on the same hand
- `tapping-term`: 280ms — intentionally large; makes behavior timer-insensitive
- `quick-tap`: 175ms — rapid same-key repeats always produce taps (e.g., `ll`)
- `require-prior-idle`: 150ms — if you typed any key less than 150ms ago, it always types the letter

---

## 🔢 Layer 1: Numbers

Activated by holding the **left thumb middle key** (NUM).

```
┌──────┬──────┬──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┬──────┬──────┐
│ ESC  │      │      │      │      │NumLck│  │  /   │  7   │  8   │  9   │  -   │ BKSP │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│CT/Tab│      │      │      │      │      │  │  *   │  4   │  5   │  6   │  ,   │      │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ SHFT │      │      │      │      │      │  │      │  1   │  2   │  3   │  =   │      │
└──────┴──────┴──┬───┴──┬───┴──┬───┴──┬───┘  └───┬──┴──┬───┴──┬───┴──┬───┴──────┴──────┘
                 │      │(held)│ TAB  │           │     │CHARS │  0   │
                 └──────┴──────┴──────┘           └─────┴──────┴──────┘
```

> **Tri-layer:** While holding NUM, also hold CHARS (right thumb) → activates **Adjust** layer.

---

## 🔣 Layer 2: Symbols

Activated by holding the **right thumb middle key** (CHARS).

> **Note:** This layer uses Swedish-specific HID keycodes (e.g., `RS(N6)` for `&`,
> `RA(N2)` for `@`). The comments show the actual character produced on a
> Swedish OS keyboard layout.

```
┌──────┬──────┬──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┬──────┬──────┐
│  &   │  *   │  @   │  {   │  }   │  |   │  │  +   │  -   │      │      │  =   │ DEL  │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│  #   │      │  $   │  (   │  )   │  `   │  │  !   │  _   │      │  /   │  \   │  '   │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│  %   │      │  ^   │  [   │  ]   │  ~   │  │  ?   │  =   │  ,   │  <   │  >   │  "   │
└──────┴──────┴──┬───┴──┬───┴──┬───┴──┬───┘  └───┬──┴──┬───┴──┬───┴──┬───┴──────┴──────┘
                 │      │ NUM  │      │           │     │(held)│      │
                 └──────┴──────┴──────┘           └─────┴──────┴──────┘
```

> **Tri-layer:** While holding CHARS, also hold NUM (left thumb) → activates **Adjust** layer.

---

## 🧭 Layer 3: Navigation

Activated by **holding E** on the main layer.

```
┌──────┬──────┬──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┬──────┬──────┐
│      │      │      │      │      │      │  │      │PG DN │  ↑   │PG UP │      │PrtScn│
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ CTRL │      │      │      │      │      │  │←WORD │  ←   │  ↓   │  →   │WORD→ │CAPS  │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ SHFT │      │      │      │      │      │  │      │ HOME │      │ END  │      │      │
└──────┴──────┴──┬───┴──┬───┴──┬───┴──┬───┘  └───┬──┴──┬───┴──┬───┴──┬───┴──────┴──────┘
                 │ GUI  │      │ ALT  │           │     │      │      │
                 └──────┴──────┴──────┘           └─────┴──────┴──────┘
```

**Word-jump:** The keys flanking the arrow cluster (←WORD and WORD→) send `Ctrl+Left` and `Ctrl+Right`, letting you jump word-by-word through text.

---

## ⚙️ Layer 4: Adjust (Tri-layer)

Activated by holding **NUM + CHARS** simultaneously (both thumb middle keys).

```
┌──────┬──────┬──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┬──────┬──────┐
│      │ BT 0 │ BT 1 │ BT 2 │ BT 3 │ BT 4 │  │BT CLR│ REW  │      │ FFWD │BRI ↑│VOL ↑│
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│      │      │      │      │      │      │  │      │ PREV │ PLAY │ NEXT │BRI ↓│VOL ↓│
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│      │      │      │      │      │      │  │      │      │      │      │      │ MUTE │
└──────┴──────┴──┬───┴──┬───┴──┬───┴──┬───┘  └───┬──┴──┬───┴──┬───┴──┬───┴──────┴──────┘
                 │ GUI  │      │ ALT  │           │     │      │      │
                 └──────┴──────┴──────┘           └─────┴──────┴──────┘
```

**Bluetooth:** BT 0–4 selects a profile. BT CLR clears the current profile's pairing.

---

## 🎯 Layer 5: Function Keys

Activated by **holding B** on the main layer.

```
┌──────┬──────┬──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┬──────┬──────┐
│      │      │      │      │      │      │  │      │  F7  │  F8  │  F9  │ F12  │PrtScn│
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ CTRL │      │      │      │      │      │  │      │  F6  │  F5  │  F4  │ F11  │CAPS  │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ SHFT │      │      │      │      │      │  │      │  F3  │  F2  │  F1  │      │      │
└──────┴──────┴──┬───┴──┬───┴──┬───┴──┬───┘  └───┬──┴──┬───┴──┬───┴──┬───┴──────┴──────┘
                 │ GUI  │      │ ALT  │           │     │      │ F10  │
                 └──────┴──────┴──────┘           └─────┴──────┴──────┘
```

---

## 🪟 Layer 6: Tiling (Window Management)

Activated by **holding R** on the main layer. Uses `Ctrl+Alt+key` shortcuts for a tiling window manager.

```
┌──────┬──────┬──────┬──────┬──────┬──────┐  ┌──────┬──────┬──────┬──────┬──────┬──────┐
│      │      │      │      │      │      │  │  +   │  UL  │  UP  │  UR  │ F12  │PrtScn│
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ CTRL │      │      │      │      │      │  │      │  ML  │  MID │  MR  │ F11  │CAPS  │
├──────┼──────┼──────┼──────┼──────┼──────┤  ├──────┼──────┼──────┼──────┼──────┼──────┤
│ SHFT │      │      │      │      │      │  │  -   │  BL  │  DN  │  BR  │      │      │
└──────┴──────┴──┬───┴──┬───┴──┬───┴──┬───┘  └───┬──┴──┬───┴──┬───┴──┬───┴──────┴──────┘
                 │ GUI  │      │ ALT  │           │     │      │ F10  │
                 └──────┴──────┴──────┘           └─────┴──────┴──────┘
```

Positions: UL=Upper Left, UP=Upper Middle, UR=Upper Right, ML=Middle Left, MID=Center, MR=Middle Right, BL=Bottom Left, DN=Bottom Middle, BR=Bottom Right. `+`/`-` grow/shrink windows.

---

## 🎮 Layer 7: Gaming

Toggle on/off with **U + P** combo (press both simultaneously).

Same as Main layer but **left hand home row mods are disabled** — S, D, F type letters directly with no hold delay. This is essential for WASD gaming. Right hand still has HRMs.

---

## ⚡ Combos (press keys simultaneously)

| Keys           | Action             | Where it works        |
|----------------|--------------------|-----------------------|
| **Z + X**      | Undo (Ctrl+Z)      | All layers except Game|
| **X + C**      | Copy (Ctrl+C)      | All layers except Game|
| **C + V** (V+C)| Paste (Ctrl+V)     | All layers except Game|
| **V + X**      | Cut (Ctrl+X)       | All layers except Game|
| **U + P**      | Toggle Gaming mode  | Main and Game (must pause typing 500ms first, press within 25ms) |
| **G + H**      | Caps Word           | Main and Game layers  |
| **Space + Enter** | Sticky Shift     | Main and Game layers  |

> **Combo timeout:** 40ms — press both keys within this window.

---

## 🧠 Special Behaviors

### Caps Word (G + H)

Temporarily activates caps lock for **one word only**. Perfect for typing `CONSTANT_NAMES`:

1. Press **G + H** together → caps word activates
2. Type your word — all letters come out UPPERCASE
3. It automatically deactivates when you press space, enter, or any non-letter/non-underscore key

Example: `G+H` then type `my_constant` → outputs `MY_CONSTANT`

### Sticky Shift (Space + Enter)

One-shot shift — press the combo, then the **next single keypress** will be shifted:

1. Press **Space + Enter** together (left and right thumb keys)
2. Type one character → it comes out shifted (uppercase or symbol)
3. Shift automatically deactivates after that one keypress

Useful for capitalizing a single letter without holding shift.

### Tri-layer (NUM + CHARS → Adjust)

Instead of needing a dedicated key for the Adjust layer, just hold both layer keys:

1. Hold **left thumb middle** (NUM layer active)
2. While still holding, also hold **right thumb middle** → **Adjust** layer activates
3. Release either thumb → drops back to Numbers or Symbols

---

## ⚡ Performance Settings

Located in `config/corne.conf`:

| Setting | Value | Purpose |
|---------|-------|---------|
| Press debounce | 1ms | Near-instant key registration (eager debounce) |
| Release debounce | 5ms | Standard release filtering |
| BT min interval | 6 | Locks BT polling for consistent wireless latency |
| Sleep timeout | 30 min | Deep sleep after inactivity to save battery |

---

## 🐣 Tamagotchi Animation (nice!view)

An optional animated pet widget for the right-half nice!view display. When enabled it replaces the default status screen with expressive EMO-style eyes that blink and react.

### Enabling

Uncomment the following lines:

1. In `config/corne.conf` — enable the display and disable default ZMK widgets
   (required to avoid linker conflicts with nice_view's own status screen):
   ```
   CONFIG_ZMK_DISPLAY=y
   CONFIG_ZMK_WIDGET_LAYER_STATUS=n
   CONFIG_ZMK_WIDGET_BATTERY_STATUS=n
   CONFIG_ZMK_WIDGET_OUTPUT_STATUS=n
   CONFIG_ZMK_WIDGET_WPM_STATUS=n
   ```

2. In `config/corne_right.conf` — swap the default widget for the tamagotchi:
   ```
   CONFIG_NICE_VIEW_WIDGET_STATUS=n
   CONFIG_TAMAGOTCHI_WIDGET=y
   ```

3. Rebuild and flash **both halves** (left needs display enabled, right needs the widget).

### Disabling

Re-comment (add `#` prefix to) the same lines and rebuild. The display will either show the default nice!view status screen (if you keep `CONFIG_ZMK_DISPLAY=y` and re-enable the default widgets) or turn off entirely.

---

## 📁 File Structure

```
config/
├── corne.keymap        # All layers, behaviors, combos
├── corne.conf          # Board settings (debounce, BT, sleep, display)
├── corne_right.conf    # Right-half overrides (tamagotchi widget)
├── west.yml            # ZMK and module dependencies
├── zephyr/module.yml   # Registers the tamagotchi Zephyr module
└── tamagotchi/         # Tamagotchi widget source
    ├── CMakeLists.txt
    ├── Kconfig
    └── src/
        ├── status_screen.c
        ├── tamagotchi_widget.c
        └── tamagotchi_widget.h
build.yaml              # CI build matrix (board/shield combos)
```
