#!/usr/bin/env python3
"""Convert clawd-on-desk Clawd GIF assets to ESP32 RGB565 raw frames."""

import os, struct, sys
from PIL import Image

GIF_DIR = "/tmp/clawd-on-desk/assets/gif"
OUT_DIR = "/Users/pjw/dev/project/esp32-clawd/firmware/assets"
W, H = 240, 240

# Map our 12 states to Clawd GIF files
STATE_MAP = {
    "idle":             "clawd-idle.gif",
    "thinking":         "clawd-thinking.gif",
    "typing":           "clawd-typing.gif",
    "building":         "clawd-building.gif",
    "error":            "clawd-error.gif",
    "happy":            "clawd-happy.gif",
    "sleeping":         "clawd-sleeping.gif",
    "subagent":         "clawd-headphones-groove.gif",
    "notification":     "clawd-notification.gif",
    "sweeping":         "clawd-sweeping.gif",
    "carrying":         "clawd-carrying.gif",
    "subagent_multi":   "clawd-juggling.gif",
}

def gif_to_frames(path, max_frames=8):
    """Extract frames from a GIF, skipping to keep ~max_frames total."""
    img = Image.open(path)
    all_frames = []
    try:
        while True:
            frame = img.convert("RGBA")
            all_frames.append(frame.copy())
            img.seek(img.tell() + 1)
    except EOFError:
        pass

    if len(all_frames) <= max_frames:
        return all_frames, img.info.get("duration", 100)

    # Sample evenly
    step = len(all_frames) / max_frames
    picked = []
    for i in range(max_frames):
        idx = int(i * step)
        picked.append(all_frames[idx])
    return picked, img.info.get("duration", 100)

def resize_fill(frame, size):
    """Resize frame to size, maintaining aspect ratio with padding."""
    fw, fh = frame.size
    scale = min(size[0] / fw, size[1] / fh)
    scale *= 1.25  # make Clawd 25% larger
    new_w, new_h = int(fw * scale), int(fh * scale)
    resized = frame.resize((new_w, new_h), Image.LANCZOS)
    canvas = Image.new("RGBA", size, (26, 28, 40, 255))  # light gray bg
    ox = (size[0] - new_w) // 2
    oy = (size[1] - new_h) // 2
    canvas.paste(resized, (ox, oy), resized)
    return canvas

def to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def frame_to_raw(frame):
    """Convert RGBA PIL Image to RGB565 bytes (big-endian for SPI)."""
    bg = Image.new("RGBA", frame.size, (26, 28, 40, 255))
    bg.paste(frame, (0, 0), frame)
    rgb = bg.convert("RGB")
    raw = bytearray(W * H * 2)
    pixels = list(rgb.getdata())
    for i, (r, g, b) in enumerate(pixels):
        val = to_rgb565(r, g, b)
        # Store big-endian: MSB first for SPI
        raw[i*2] = (val >> 8) & 0xFF      # MSB
        raw[i*2+1] = val & 0xFF           # LSB
    return bytes(raw)

def main():
    for state, filename in STATE_MAP.items():
        path = os.path.join(GIF_DIR, filename)
        if not os.path.exists(path):
            print(f"  SKIP {state}: {filename} not found")
            continue

        print(f"  {state}: {filename}...", end=" ")
        frames, delay = gif_to_frames(path)
        if not frames:
            print("no frames!")
            continue

        out_dir = os.path.join(OUT_DIR, state)
        os.makedirs(out_dir, exist_ok=True)
        for old in os.listdir(out_dir):
            os.unlink(os.path.join(out_dir, old))

        for i, frame in enumerate(frames):
            resized = resize_fill(frame, (W, H))
            raw = frame_to_raw(resized)
            with open(os.path.join(out_dir, f"{i}.raw"), "wb") as f:
                f.write(raw)

        with open(os.path.join(out_dir, "frames.txt"), "w") as f:
            f.write(f"{len(frames)}:{max(delay, 80)}\n")

        kb = len(frames) * W * H * 2 // 1024
        print(f"{len(frames)} frames, {kb}KB")

    total = 0
    for state in STATE_MAP:
        d = os.path.join(OUT_DIR, state)
        if os.path.isdir(d):
            n = len([f for f in os.listdir(d) if f.endswith('.raw')])
            total += n
    print(f"\nTotal: {total} frames, ~{total * W * H * 2 // 1024}KB")
    print(f"SPIFFS usage: {total * W * H * 2 * 100 // (12 * 1024 * 1024)}% of 12MB")

if __name__ == "__main__":
    main()
