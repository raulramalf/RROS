#!/usr/bin/env python3
"""Fondos de pantalla de Lily: escenas en SVG (con desenfoques, degradados y grano) que Chromium pasa a imagen.

    python3 scripts/make-wallpapers.py [CARPETA] [ANCHOxALTO]

Deja en CARPETA (por defecto overlay/common/usr/share/wallpapers/Lily) un JPG por fondo y una miniatura
(miniaturas/NOMBRE.jpg) para la galería de Ajustes, más un índice fondos.txt con «archivo|nombre|claro u oscuro».
Todo sale de semillas fijas: volver a ejecutarlo da los mismos fondos.
"""
import math
import os
import random
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "overlay/common/usr/share/wallpapers/Lily")
W, H = (int(x) for x in (sys.argv[2] if len(sys.argv) > 2 else "2560x1600").split("x"))
CHROME = next((p for p in ("/opt/pw-browsers/chromium-1194/chrome-linux/chrome", shutil.which("chromium") or "",
                           shutil.which("google-chrome") or "") if p and os.path.exists(p)), None)


def blur(i, s):
    return f'<filter id="{i}" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="{s}"/></filter>'


def svg(defs, body, _grain=0.07):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" style="display:block" width="{W}" height="{H}" viewBox="0 0 2560 1600" '
            f'preserveAspectRatio="xMidYMid slice"><defs>{defs}</defs>{body}</svg>')


def lily(cx, cy, s, petal="url(#pet)", stamen="#ff7a8a", sw=3.2, op=1.0):
    """El lirio de araña de la marca: seis pétalos curvos hacia atrás y estambres largos."""
    out = [f'<g transform="translate({cx} {cy}) scale({s})" opacity="{op}">']
    out.append(f'<g fill="none" stroke="{stamen}" stroke-width="{sw}" stroke-linecap="round">')
    tips = []
    for k in range(6):
        a = math.radians(-160 + k * 28 + (8 if k > 2 else 0))
        tx, ty = 150 * math.cos(a), 150 * math.sin(a) - 20
        qx, qy = 40 * math.cos(a) * 0.4 + (-25 if k < 3 else 25), -120
        out.append(f'<path d="M0,0 Q{qx:.1f},{qy:.1f} {tx:.1f},{ty:.1f}"/>')
        tips.append((tx, ty))
    out.append('</g>')
    out += [f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{sw * 1.5:.1f}" fill="{stamen}"/>' for x, y in tips]
    for k in range(6):
        a = -175 + k * 34
        out.append(f'<path transform="rotate({a})" fill="{petal}" d="M0,-3 C40,-22 80,-18 120,-6 C138,0 140,14 128,16 '
                   f'C120,8 104,6 90,12 C60,20 30,14 0,4Z"/>')
    out.append('</g>')
    return "".join(out)


# ---------------------------------------------------------------- fondos

def f_lirio():
    r = random.Random(1)
    bokeh = "".join(f'<circle cx="{r.uniform(0, 2560):.0f}" cy="{r.uniform(0, 1600):.0f}" r="{r.uniform(8, 46):.0f}" '
                    f'fill="#ff5a70" opacity="{r.uniform(0.04, 0.16):.2f}"/>' for _ in range(46))
    defs = (blur("b60", 60) + blur("b14", 14) + blur("b3", 3) +
            '<radialGradient id="bg" cx="0.68" cy="0.42" r="0.9"><stop offset="0" stop-color="#4a0c1c"/>'
            '<stop offset="0.45" stop-color="#22060e"/><stop offset="1" stop-color="#0a0205"/></radialGradient>'
            '<linearGradient id="pet" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#9e0f28"/>'
            '<stop offset="0.6" stop-color="#e5304a"/><stop offset="1" stop-color="#ff7b8c"/></linearGradient>')
    body = (f'<rect width="2560" height="1600" fill="url(#bg)"/><g filter="url(#b14)">{bokeh}</g>'
            f'<g filter="url(#b60)">{lily(1720, 900, 3.4, "#c8203c", "#c8203c", 6, 0.55)}</g>'
            f'<g filter="url(#b3)">{lily(1720, 900, 3.2)}</g>'
            # tallo fino que se pierde abajo
            '<path d="M1720,905 C1712,1150 1700,1400 1690,1640" stroke="#3b6b3a" stroke-width="10" fill="none" opacity="0.7"/>')
    return svg(defs, body, 0.09)


def f_seda(light=False):
    r = random.Random(2)
    bg = ("#f6eee6", "#ead8cc") if light else ("#16050b", "#2c0a15")
    cols = (["#e9b9a8", "#d98f88", "#f3d9c6", "#c86f75"] if light else ["#7a1328", "#c8243c", "#d9b98f", "#4a0b1a"])
    ribbons = []
    for i in range(9):
        y0 = 300 + i * 120 + r.uniform(-60, 60)
        a, b = r.uniform(-380, 380), r.uniform(-380, 380)
        c = cols[i % len(cols)]
        w = r.uniform(70, 190)
        ribbons.append(f'<path d="M-200,{y0:.0f} C700,{y0 + a:.0f} 1700,{y0 + b:.0f} 2760,{y0 + a * 0.4:.0f}" stroke="{c}" '
                       f'stroke-width="{w:.0f}" fill="none" opacity="{r.uniform(0.35, 0.75):.2f}" stroke-linecap="round"/>')
        # brillo de la tela: una línea clara y fina sobre cada pliegue
        ribbons.append(f'<path d="M-200,{y0 - w / 3:.0f} C700,{y0 + a - w / 3:.0f} 1700,{y0 + b - w / 3:.0f} 2760,{y0 + a * 0.4 - w / 3:.0f}" '
                       f'stroke="#fff6ee" stroke-width="{w / 7:.0f}" fill="none" opacity="{0.35 if light else 0.12}"/>')
    defs = (blur("b30", 30) + blur("b6", 6) +
            f'<linearGradient id="bg" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="{bg[0]}"/>'
            f'<stop offset="1" stop-color="{bg[1]}"/></linearGradient>')
    body = (f'<rect width="2560" height="1600" fill="url(#bg)"/><g filter="url(#b30)">{"".join(ribbons[::2])}</g>'
            f'<g filter="url(#b6)" transform="rotate(-8 1280 800)">{"".join(ribbons)}</g>')
    return svg(defs, body, 0.06)


def ridge(r, y, amp, n=7):
    """Silueta de cordillera: desplazamiento del punto medio, con picos y laderas irregulares."""
    pts = [(-60.0, y + r.uniform(-amp, amp) * 0.5), (2620.0, y + r.uniform(-amp, amp) * 0.5)]
    a = amp
    for _ in range(n):
        nxt = []
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            nxt += [(x0, y0), ((x0 + x1) / 2, (y0 + y1) / 2 + r.uniform(-a, a))]
        pts = nxt + [pts[-1]]
        a *= 0.55
    return "M-60,1700 " + " ".join(f"L{x:.0f},{yy:.0f}" for x, yy in pts) + " L2620,1700Z"


def f_crepusculo():
    r = random.Random(3)
    stars = "".join(f'<circle cx="{r.uniform(0, 2560):.0f}" cy="{r.uniform(0, 650):.0f}" r="{r.uniform(0.8, 2.4):.1f}" '
                    f'fill="#fff" opacity="{r.uniform(0.2, 0.9):.2f}"/>' for _ in range(260))
    layers = ["#3a1530", "#2c1028", "#200b20", "#150716", "#0c040d"]
    hills = "".join(f'<path d="{ridge(r, 860 + i * 140, 260 - i * 40)}" fill="{c}"/>'
                    f'<rect y="{900 + i * 140}" width="2560" height="160" fill="#ff9a8a" opacity="{0.10 - i * 0.015:.3f}" filter="url(#b40)"/>'
                    for i, c in enumerate(layers))
    defs = (blur("b40", 40) + blur("b120", 120) +
            '<linearGradient id="sky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#0d0a24"/>'
            '<stop offset="0.45" stop-color="#3d1a45"/><stop offset="0.7" stop-color="#b8475a"/>'
            '<stop offset="0.86" stop-color="#ffb38a"/><stop offset="1" stop-color="#ffd9b0"/></linearGradient>')
    body = (f'<rect width="2560" height="1600" fill="url(#sky)"/>{stars}'
            '<circle cx="1820" cy="905" r="150" fill="#ffd2a4" opacity="0.9" filter="url(#b40)"/>'
            '<circle cx="1820" cy="905" r="420" fill="#ff8f7a" opacity="0.35" filter="url(#b120)"/>'
            f'{hills}')
    return svg(defs, body, 0.07)


def f_marfil():
    r = random.Random(4)
    waves = "".join(f'<path d="M-100,{700 + i * 110} C600,{560 + i * 120 + r.uniform(-80, 80):.0f} 1500,{900 + i * 90:.0f} 2700,{620 + i * 115}" '
                    f'stroke="{c}" stroke-width="{r.uniform(60, 170):.0f}" fill="none" opacity="0.8"/>'
                    for i, c in enumerate(["#efcfc0", "#e7b8ab", "#f1dcc9", "#dfa79e", "#ecd2bd", "#d99795"]))
    defs = (blur("b50", 50) + blur("b10", 10) +
            '<linearGradient id="bg" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#fbf7f1"/>'
            '<stop offset="1" stop-color="#f1e4d8"/></linearGradient>'
            '<linearGradient id="pet" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#b3112c"/>'
            '<stop offset="1" stop-color="#ef5468"/></linearGradient>')
    body = (f'<rect width="2560" height="1600" fill="url(#bg)"/><g filter="url(#b50)">{waves}</g>'
            f'<g filter="url(#b10)" opacity="0.6">{waves}</g>'
            # lirio grande en relieve, muy suave, como una marca de agua
            f'<g filter="url(#b10)">{lily(1900, 1120, 4.2, "#e9c4b8", "#e2b2a6", 4, 0.75)}</g>'
            f'{lily(1900, 1120, 4.2, "none", "#d7a597", 1.4, 0.5)}')
    return svg(defs, body, 0.05)


def f_bruma():
    r = random.Random(5)
    cols = ["#ffc4c9", "#ffd9b8", "#e9c6ff", "#c9d8ff", "#ffb3c0", "#fbe2d0"]
    blobs = "".join(f'<ellipse cx="{r.uniform(0, 2560):.0f}" cy="{r.uniform(0, 1600):.0f}" rx="{r.uniform(380, 820):.0f}" '
                    f'ry="{r.uniform(260, 600):.0f}" fill="{cols[i % 6]}" opacity="0.85"/>' for i in range(14))
    defs = blur("b160", 160)
    body = f'<rect width="2560" height="1600" fill="#fbf1ec"/><g filter="url(#b160)">{blobs}</g>'
    return svg(defs, body, 0.05)


def f_oceano():
    r = random.Random(6)
    swells = []
    for i in range(7):
        y = 860 + i * 110
        pts = " ".join(f"S{x + 180:.0f},{y + r.uniform(-60, 30):.0f} {x + 360:.0f},{y + r.uniform(-25, 25):.0f}" for x in range(-200, 2800, 360))
        d = f"M-200,{y} {pts} L2800,1700 L-200,1700Z"
        swells.append(f'<path d="{d}" fill="url(#w{i})"/>'
                      f'<linearGradient id="w{i}" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="rgb({8 + i * 2},{52 + i * 6},{70 + i * 7})"/>'
                      f'<stop offset="1" stop-color="rgb(2,{14 + i * 3},{24 + i * 4})"/></linearGradient>')
    glints = "".join(f'<rect x="{1640 + r.gauss(0, 60) - w / 2:.0f}" y="{y:.0f}" width="{w:.0f}" height="3" rx="1.5" fill="#e6fbff" opacity="{r.uniform(0.25, 0.8):.2f}"/>'
                     for y, w in ((r.uniform(820, 1600), r.uniform(20, 140) * (1 - k / 260)) for k in range(200)))
    defs = (blur("b90", 90) + blur("b2", 1.2) +
            '<linearGradient id="sky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#020a14"/>'
            '<stop offset="0.45" stop-color="#0a2a3c"/><stop offset="0.56" stop-color="#2a6a78"/><stop offset="1" stop-color="#04121c"/></linearGradient>')
    stars = "".join(f'<circle cx="{r.uniform(0, 2560):.0f}" cy="{r.uniform(0, 640):.0f}" r="{r.uniform(0.7, 2):.1f}" fill="#fff" opacity="{r.uniform(0.2, 0.8):.2f}"/>' for _ in range(180))
    body = (f'<rect width="2560" height="1600" fill="url(#sky)"/>{stars}'
            '<circle cx="1640" cy="420" r="300" fill="#bfeff5" opacity="0.18" filter="url(#b90)"/>'
            '<circle cx="1640" cy="420" r="70" fill="#f1fdff"/>'
            f'{"".join(swells)}<g filter="url(#b2)">{glints}</g>')
    return svg(defs, body, 0.06)


def f_dunas():
    r = random.Random(7)
    dunes = []
    tones = [("#f2b880", "#c96f4a"), ("#e9a06c", "#b45c3c"), ("#d98a5a", "#9b4a32"), ("#c4744a", "#7f3a28"), ("#a85e3c", "#5f2a1e")]
    for i, (a, b) in enumerate(tones):
        y = 760 + i * 170
        x0 = r.uniform(300, 2200)
        d = f"M-100,1700 L-100,{y + 120} C{x0 - 700:.0f},{y + 80} {x0 - 300:.0f},{y - 160} {x0:.0f},{y - 140} C{x0 + 400:.0f},{y - 120} {x0 + 900:.0f},{y + 60} 2700,{y + 40} L2700,1700Z"
        dunes.append(f'<linearGradient id="d{i}" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="{a}"/><stop offset="1" stop-color="{b}"/></linearGradient>'
                     f'<path d="{d}" fill="url(#d{i})"/>')
    defs = (blur("b80", 80) +
            '<linearGradient id="sky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#f6d6b8"/>'
            '<stop offset="0.6" stop-color="#f7c49a"/><stop offset="1" stop-color="#eaa77a"/></linearGradient>')
    body = (f'<rect width="2560" height="1600" fill="url(#sky)"/>'
            '<circle cx="760" cy="560" r="120" fill="#fff4dc" filter="url(#b80)"/>'
            '<circle cx="760" cy="560" r="64" fill="#fffaf0" opacity="0.9"/>'
            f'{"".join(dunes)}')
    return svg(defs, body, 0.08)


def f_bosque():
    r = random.Random(8)
    layers = []
    cols = ["#5f7d72", "#45625a", "#304a45", "#1f3532", "#122322"]
    for i, c in enumerate(cols):
        base = 900 + i * 150
        trees = []
        x = -60
        while x < 2620:
            h = r.uniform(180, 420) * (1 + i * 0.25)
            w = h * r.uniform(0.26, 0.34)
            cx = x + w / 2
            # abeto de tres pisos: cada uno un poco más estrecho y con las puntas caídas
            for t in range(3):
                tb, tt = base - h * t * 0.28, base - h * (0.45 + t * 0.28)
                tw = w * (1 - t * 0.22)
                trees.append(f"M{cx - tw / 2:.0f},{tb:.0f} Q{cx - tw / 5:.0f},{tb - h * 0.08:.0f} {cx:.0f},{tt:.0f} Q{cx + tw / 5:.0f},{tb - h * 0.08:.0f} {cx + tw / 2:.0f},{tb:.0f}Z")
            x += w * r.uniform(0.45, 0.9)
        layers.append(f'<g fill="{c}"><path d="{" ".join(trees)}"/><rect y="{base - 2}" width="2560" height="900"/></g>'
                      f'<rect y="{base - 260}" width="2560" height="300" fill="url(#fog)" opacity="{0.75 - i * 0.12:.2f}"/>')
    defs = (blur("b60", 60) +
            '<linearGradient id="sky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#e4efe9"/>'
            '<stop offset="1" stop-color="#a9c4b9"/></linearGradient>'
            '<linearGradient id="fog" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#e8f2ed" stop-opacity="0"/>'
            '<stop offset="0.8" stop-color="#e8f2ed" stop-opacity="0.9"/><stop offset="1" stop-color="#e8f2ed" stop-opacity="0"/></linearGradient>')
    body = (f'<rect width="2560" height="1600" fill="url(#sky)"/>'
            '<circle cx="1900" cy="420" r="260" fill="#fffbe8" opacity="0.6" filter="url(#b60)"/>'
            f'{"".join(layers)}')
    return svg(defs, body, 0.07)


FONDOS = [
    ("lirio", "Lirio", "oscuro", f_lirio),
    ("seda", "Seda", "oscuro", f_seda),
    ("crepusculo", "Crepúsculo", "oscuro", f_crepusculo),
    ("oceano", "Océano", "oscuro", f_oceano),
    ("bosque", "Bosque", "claro", f_bosque),
    ("dunas", "Dunas", "claro", f_dunas),
    ("marfil", "Marfil", "claro", f_marfil),
    ("seda-clara", "Seda clara", "claro", lambda: f_seda(True)),
    ("bruma", "Bruma", "claro", f_bruma),
]


def add_grain(im, amount=3.2):
    """Grano fino e igual en toda la imagen: evita las bandas de los degradados oscuros al comprimir."""
    import numpy as np
    a = np.asarray(im).astype(np.float32)
    n = np.random.default_rng(11).normal(0, amount, a.shape[:2])[..., None]
    from PIL import Image
    return Image.fromarray(np.clip(a + n, 0, 255).astype(np.uint8))


def render(svg_text, out_png, w, h):
    with tempfile.TemporaryDirectory() as tmp:
        page = os.path.join(tmp, "f.html")
        with open(page, "w") as f:
            f.write(f'<html><body style="margin:0;overflow:hidden;background:#000">{svg_text}</body></html>')
        subprocess.run([CHROME, "--headless", "--no-sandbox", "--disable-gpu", "--hide-scrollbars",
                        f"--window-size={w},{h + 200}", f"--screenshot={out_png}", "file://" + page],
                       check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def main():
    if not CHROME:
        sys.exit("make-wallpapers: hace falta Chromium para pasar los SVG a imagen")
    from PIL import Image
    os.makedirs(os.path.join(OUT, "miniaturas"), exist_ok=True)
    index = []
    for slug, name, tone, fn in FONDOS:
        with tempfile.TemporaryDirectory() as tmp:
            png = os.path.join(tmp, "f.png")
            render(fn(), png, W, H)
            im = add_grain(Image.open(png).convert("RGB").crop((0, 0, W, H)))
            im.save(os.path.join(OUT, slug + ".jpg"), quality=88, optimize=True, progressive=True)
            im.resize((384, 240), Image.LANCZOS).save(os.path.join(OUT, "miniaturas", slug + ".jpg"), quality=86)
        index.append(f"{slug}.jpg|{name}|{tone}")
        print("fondo:", slug)
    with open(os.path.join(OUT, "fondos.txt"), "w") as f:
        f.write("\n".join(index) + "\n")


if __name__ == "__main__":
    main()
