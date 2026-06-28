#!/usr/bin/env python3
"""
Generate placeholder RGB565 raw frame assets for all animation states.

Each state gets a directory under firmware/assets/ with:
  - frames.txt  — first line: frame count, second line: delay in ms
  - 0.raw, 1.raw, … — RGB565 raw frames (240x240 = 115200 bytes each)

Multi-frame states (idle, thinking, typing) include a small moving circle
for a simple visual animation effect.

Usage:
    python3 firmware/assets/generate_placeholders.py
"""

import math
import os
import struct
import sys

WIDTH = 240
HEIGHT = 240
FRAME_SIZE = WIDTH * HEIGHT * 2  # RGB565 = 2 bytes/pixel
DELAY_MS = 150

# R, G, B tuples for each state (distinct solid colors)
STATES = {
    "idle":           (52, 152, 219),    # blue
    "thinking":       (241, 196, 15),    # yellow
    "typing":         (46, 204, 113),    # green
    "building":       (230, 126, 34),    # orange
    "error":          (231, 76, 60),     # red
    "happy":          (155, 89, 182),    # purple
    "sleeping":       (127, 140, 141),   # gray
    "subagent":       (26, 188, 156),    # teal
    "notification":   (243, 156, 18),    # amber
    "sweeping":       (41, 128, 185),    # dark blue
    "carrying":       (142, 68, 173),    # dark purple
    "subagent_multi": (22, 160, 133),    # dark teal
}

# Multi-frame states with moving circle offsets (x, y center positions per frame)
# Single-frame states get just a solid color.
MULTI_FRAME = {
    "idle":     [(60, 120), (120, 120), (180, 120)],      # 3 frames, circle moves horizontally
    "thinking": [(120, 80), (120, 120), (120, 160)],      # 3 frames, circle moves vertically
    "typing":   [(80, 120),  (160, 120)],                  # 2 frames, circle jumps horizontally
}

CIRCLE_RADIUS = 20
CIRCLE_COLOR_RGB = (255, 255, 255)  # white circle
CIRCLE_COLOR_565 = None  # computed below


def rgb_to_565(r, g, b):
    """Convert 8-bit R,G,B to RGB565 (uint16)."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def make_solid_frame(r, g, b):
    """Create a list of uint16 pixel values for a solid-color frame."""
    color = rgb_to_565(r, g, b)
    return [color] * (WIDTH * HEIGHT)


def make_circle_frame(r, g, b, cx, cy, radius, circle_color):
    """Create a list of uint16 pixel values with a filled circle overlay."""
    bg = rgb_to_565(r, g, b)
    pixels = bytearray(FRAME_SIZE)

    # Fill background first (fast: use struct.pack for each row)
    for y in range(HEIGHT):
        offset = y * WIDTH * 2
        for x in range(WIDTH):
            # Check if pixel is inside the circle
            dx = x - cx
            dy = y - cy
            if dx * dx + dy * dy <= radius * radius:
                struct.pack_into("<H", pixels, offset + x * 2, circle_color)
            else:
                struct.pack_into("<H", pixels, offset + x * 2, bg)
    return pixels


def write_raw_file(path, frame_data):
    """Write frame data to a raw file. Accepts either a list of ints or a bytearray."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    if isinstance(frame_data, bytearray):
        with open(path, "wb") as f:
            f.write(frame_data)
    else:
        # Write as little-endian uint16 array
        with open(path, "wb") as f:
            for pixel in frame_data:
                f.write(struct.pack("<H", pixel & 0xFFFF))


def main():
    base = os.path.abspath(os.path.dirname(__file__))
    circle_color = rgb_to_565(*CIRCLE_COLOR_RGB)

    total_frames = 0

    for state, (r, g, b) in STATES.items():
        state_dir = os.path.join(base, state)
        os.makedirs(state_dir, exist_ok=True)

        # Determine frame count and build frames
        circle_centers = MULTI_FRAME.get(state, None)

        if circle_centers:
            count = len(circle_centers)
        else:
            count = 1

        for i in range(count):
            path = os.path.join(state_dir, f"{i}.raw")
            if circle_centers:
                cx, cy = circle_centers[i]
                frame_data = make_circle_frame(r, g, b, cx, cy,
                                               CIRCLE_RADIUS, circle_color)
                print(f"  {path}  (circle at {cx},{cy})")
            else:
                frame_data = make_solid_frame(r, g, b)
                print(f"  {path}  (solid)")
            write_raw_file(path, frame_data)

        # Write frames.txt metadata
        meta_path = os.path.join(state_dir, "frames.txt")
        with open(meta_path, "w") as f:
            f.write(f"{count}:{DELAY_MS}\n")
        print(f"  {meta_path}  ({count} frames, {DELAY_MS}ms delay)")

        total_frames += count
        print()

    print(f"Generated {total_frames} total frames across "
          f"{len(STATES)} states in {base}")


if __name__ == "__main__":
    main()
