#!/usr/bin/env python3
"""Convert clawd-on-desk Calico APNG assets to ESP32 RGB565 raw frames."""

import os, struct, sys
from PIL import Image

CLAWD_DIR = "/tmp/clawd-on-desk/themes/calico/assets"
OUT_DIR = "/Users/pjw/dev/project/esp32-clawd/firmware/assets"
W, H = 240, 240

# Map clawd-on-desk APNGs to our 12 state names
STATE_MAP = {
    "idle":          "calico-idle.apng",
    "thinking":      "calico-thinking.apng",
    "typing":        "calico-working-typing.apng",
    "building":      "calico-working-building.apng",
    "error":         "calico-error.apng",
    "happy":         "calico-happy.apng",
    "sleeping":      "calico-sleeping.apng",
    "subagent":      "calico-working-juggling.apng",
    "notification":  "calico-notification.apng",
    "sweeping":      "calico-working-sweeping.apng",
    "carrying":      "calico-working-carrying.apng",
    "subagent_multi":"calico-working-conducting.apng",
}

def apng_to_frames(path, max_frames=8):
    """Extract frames from an APNG, skipping to keep ~max_frames total."""
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
        return all_frames

    # Sample evenly
    step = len(all_frames) / max_frames
    picked = []
    for i in range(max_frames):
        idx = int(i * step)
        picked.append(all_frames[idx])
    return picked

def resize_fill(frame, size):
    """Resize frame to size, maintaining aspect ratio with transparent padding."""
    fw, fh = frame.size
    scale = min(size[0] / fw, size[1] / fh)
    new_w, new_h = int(fw * scale), int(fh * scale)
    resized = frame.resize((new_w, new_h), Image.LANCZOS)

    # Center on transparent canvas
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    ox = (size[0] - new_w) // 2
    oy = (size[1] - new_h) // 2
    canvas.paste(resized, (ox, oy), resized)
    return canvas

def to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def frame_to_raw(frame):
    """Convert RGBA PIL Image to RGB565 bytes with light gray background."""
    bg = Image.new("RGBA", frame.size, (220, 220, 225, 255))
    bg.paste(frame, (0, 0), frame)
    rgb = bg.convert("RGB")
    raw = bytearray(W * H * 2)
    pixels = list(rgb.getdata())
    for i, (r, g, b) in enumerate(pixels):
        val = to_rgb565(r, g, b)
        val = ((val & 0xFF) << 8) | ((val >> 8) & 0xFF)  # swap to big-endian for SPI
        raw[i*2] = val & 0xFF
        raw[i*2+1] = (val >> 8) & 0xFF
    return bytes(raw)

def main():
    for state, filename in STATE_MAP.items():
        path = os.path.join(CLAWD_DIR, filename)
        if not os.path.exists(path):
            print(f"  SKIP {state}: {filename} not found")
            continue

        print(f"  {state}: {filename}...", end=" ")
        frames = apng_to_frames(path)
        if not frames:
            print("no frames!")
            continue

        out_dir = os.path.join(OUT_DIR, state)
        os.makedirs(out_dir, exist_ok=True)

        # Clear old files
        for old in os.listdir(out_dir):
            os.unlink(os.path.join(out_dir, old))

        # Resize and save frames
        for i, frame in enumerate(frames):
            resized = resize_fill(frame, (W, H))
            raw = frame_to_raw(resized)
            fpath = os.path.join(out_dir, f"{i}.raw")
            with open(fpath, "wb") as f:
                f.write(raw)

        # Write metadata
        delay_ms = 100  # default
        try:
            img = Image.open(path)
            delay_ms = img.info.get("duration", 100)
        except:
            pass

        with open(os.path.join(out_dir, "frames.txt"), "w") as f:
            f.write(f"{len(frames)}:{delay_ms}\n")

        total_kb = sum(len(frame_to_raw(resize_fill(f, (W, H)))) for f in frames) // 1024
        print(f"{len(frames)} frames @ {W}x{H}, {total_kb}KB")

    # Print summary
    total = 0
    for state in STATE_MAP:
        d = os.path.join(OUT_DIR, state)
        if os.path.isdir(d):
            n = len([f for f in os.listdir(d) if f.endswith('.raw')])
            total += n
            print(f"  {state}: {n} frames")
    print(f"\nTotal: {total} frames across {len(STATE_MAP)} states")

if __name__ == "__main__":
    main()
