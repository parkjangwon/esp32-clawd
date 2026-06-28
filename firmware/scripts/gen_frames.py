#!/usr/bin/env python3
"""
Generate placeholder RGB565 frame files for each animation state.

Each state gets a directory under `assets/` with numbered .raw frame files
and a frames.txt metadata file.

Usage:
    python3 scripts/gen_frames.py [output_dir]

If output_dir is omitted, defaults to `firmware/assets`.
"""

import os
import struct
import sys

WIDTH = 240
HEIGHT = 240
FRAME_SIZE = WIDTH * HEIGHT * 2  # RGB565 = 2 bytes/pixel

# Colors in RGB565 format (R:5, G:6, B:5)
RGB565 = {
    "black":    0x0000,
    "blue":     0x001F,
    "green":    0x07E0,
    "red":      0xF800,
    "white":    0xFFFF,
    "cyan":     0x07FF,
    "yellow":   0xFFE0,
    "purple":   0xF81F,
    "orange":   0xFD20,
    "darkblue": 0x000F,
    "gray":     0x7BEF,
    "teal":     0x780F,
    "pink":     0xFC9F,
    "lime":     0x9E73,
    "navy":     0x0010,
}

# Each state maps to a list of (color_name, color_rgb565) frames.
# Multiple frames = simple animation.
STATES = {
    "idle":           [("blue", RGB565["blue"])],
    "thinking":       [("cyan", RGB565["cyan"]), ("white", RGB565["white"]), ("cyan", RGB565["cyan"])],
    "typing":         [("green", RGB565["green"]), ("lime", RGB565["lime"])],
    "building":       [("yellow", RGB565["yellow"]), ("orange", RGB565["orange"])],
    "error":          [("red", RGB565["red"])],
    "happy":          [("purple", RGB565["purple"])],
    "sleeping":       [("navy", RGB565["navy"])],
    "subagent":       [("teal", RGB565["teal"])],
    "notification":   [("orange", RGB565["orange"])],
    "sweeping":       [("gray", RGB565["gray"])],
    "carrying":       [("lime", RGB565["lime"])],
    "subagent_multi": [("pink", RGB565["pink"])],
}

DEFAULT_DELAY_MS = 150  # default per-frame delay


def write_raw_file(path, color_rgb565):
    """Write a solid-color RGB565 frame image."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pixel = struct.pack("<H", color_rgb565 & 0xFFFF)
    with open(path, "wb") as f:
        for _ in range(WIDTH * HEIGHT):
            f.write(pixel)


def main():
    if len(sys.argv) > 1:
        base = sys.argv[1]
    else:
        base = os.path.join(os.path.dirname(__file__), "..", "assets")
    base = os.path.abspath(base)

    for state, frames in STATES.items():
        state_dir = os.path.join(base, state)
        os.makedirs(state_dir, exist_ok=True)

        count = len(frames)
        for i, (name, color) in enumerate(frames):
            path = os.path.join(state_dir, f"{i}.raw")
            write_raw_file(path, color)
            print(f"  {path}  ({name})")

        meta_path = os.path.join(state_dir, "frames.txt")
        with open(meta_path, "w") as f:
            f.write(f"{count}:{DEFAULT_DELAY_MS}\n")
        print(f"  {meta_path}  ({count} frames, {DEFAULT_DELAY_MS}ms)")

    print(f"\nGenerated {sum(len(v) for v in STATES.values())} frames "
          f"across {len(STATES)} states in {base}")


if __name__ == "__main__":
    main()
