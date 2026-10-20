#!/usr/bin/env python3
"""Vista previa (preview.png) de cada tema de cursor de RROS: flecha, mano y espera en fila, para RROS Ajustes.
Lee los propios archivos Xcursor. uso: cursor-preview.py DIR_TEMA..."""
import struct
import sys

from PIL import Image


def frame(path, size=48):
    d = open(path, "rb").read()
    n = struct.unpack_from("<I", d, 12)[0]
    best = None
    for i in range(n):
        t, sub, pos = struct.unpack_from("<III", d, 16 + i * 12)
        if t == 0xFFFD0002 and (best is None or abs(sub - size) < abs(best[0] - size)):
            best = (sub, pos)
    w, h = struct.unpack_from("<II", d, best[1] + 16)
    px = struct.unpack_from(f"<{w * h}I", d, best[1] + 36)
    im = Image.new("RGBA", (w, h))
    im.putdata([((p >> 16) & 255, (p >> 8) & 255, p & 255, p >> 24) for p in px])
    return im


for theme in sys.argv[1:]:
    out = Image.new("RGBA", (144, 48))
    for i, name in enumerate(("default", "pointer", "wait")):
        im = frame(f"{theme}/cursors/{name}")
        out.alpha_composite(im.resize((48, 48)) if im.size != (48, 48) else im, (i * 48, 0))
    out.save(f"{theme}/preview.png")
