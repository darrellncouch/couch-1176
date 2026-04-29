#!/usr/bin/env python3
"""
Generate vu_sprite.png  — a horizontal sprite sheet of the VU meter face.

Frame count : 121  (one per degree, −60° … +60° in JUCE clock convention)
Frame size  : 184 × 158 px  (matches VM_W × VM_H in PluginEditor.h)

Needle geometry (matching PluginEditor.h):
  pivot   = (frame_w/2, frame_h - 4)   i.e. (92, 154)
  NEEDLE_R = 66 px
  angle    = 0  → 12-o'clock (vertical up)
  tipX = pivotX + sin(angle) * NEEDLE_R
  tipY = pivotY - cos(angle) * NEEDLE_R   (JUCE clock-face convention)

Sweep: angle −π/3 (left, 20 dB GR) … +π/3 (right, 0 dB GR)
"""

import math
from PIL import Image, ImageDraw

SRC      = "needleless-vu.png"
OUT      = "vu_sprite.png"
FRAMES   = 121          # 0..120  (1° per frame)
FW, FH   = 184, 158     # frame size = VM_W × VM_H
PX, PY   = FW // 2, 137    # pivot in frame coords  (92, 137) — matches image pivot
NR       = 91           # NEEDLE_R — tip lands on scale endpoint at ±60°
NR_TAIL  = 12           # tail behind pivot
SWEEP    = math.pi / 3  # 60°

# Load + resize face background
face_src = Image.open(SRC).convert("RGBA")
face     = face_src.resize((FW, FH), Image.LANCZOS)

frames = []
for i in range(FRAMES):
    t     = i / (FRAMES - 1)           # 0 → 1
    angle = -SWEEP + t * 2 * SWEEP     # −π/3 → +π/3

    sa, ca = math.sin(angle), math.cos(angle)

    tip_x  = PX + sa * NR
    tip_y  = PY - ca * NR
    tail_x = PX - sa * NR_TAIL
    tail_y = PY + ca * NR_TAIL
    mid_x  = PX + sa * NR * 0.68
    mid_y  = PY - ca * NR * 0.68

    frame = face.copy()
    d     = ImageDraw.Draw(frame)

    # Shadow (1px offset)
    d.line([(PX + 1, PY + 1), (tip_x + 1, tip_y + 1)],
           fill=(0, 0, 0, 77), width=2)

    # Needle body (dark, from tail through pivot to 68% of tip)
    d.line([(round(tail_x), round(tail_y)),
            (round(mid_x),  round(mid_y))],
           fill=(10, 6, 0, 255), width=2)

    # Needle tip accent (dark brown)
    d.line([(round(mid_x), round(mid_y)),
            (round(tip_x), round(tip_y))],
           fill=(42, 8, 0, 255), width=1)

    frames.append(frame)

# Stitch frames horizontally
sheet = Image.new("RGBA", (FW * FRAMES, FH))
for i, f in enumerate(frames):
    sheet.paste(f, (i * FW, 0))

sheet.save(OUT, compress_level=6)
print(f"Saved {OUT}  ({FW * FRAMES} × {FH} px,  {FRAMES} frames)")
