#!/usr/bin/env python3
"""Tema de escritorio de Plasma de RROS: el fondo de la barra, el dock, los menús emergentes y los avisos.

Dos variantes, que rros-theme elige con ui.dock_style:
  rros     2D: plano, translúcido, esquinas redondeadas y un borde fino.
  rros-3d  3D: el mismo color con brillo arriba, bisel y un canto delante, como un estante de cristal.

Plasma dibuja cada fondo en nueve trozos (FrameSvg: topleft, top, ..., center) y estira los bordes y el centro;
los colores salen del esquema de colores en uso (clase ColorScheme-Background), así sirven en claro y en oscuro.
QtSvg no tiene filtros: el brillo y las sombras son degradados.

uso: make-desktoptheme.py  (escribe overlay/arch/usr/share/plasma/desktoptheme/{rros,rros-3d})
"""
import json
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "overlay/arch/usr/share/plasma/desktoptheme")

STYLE = """<style type="text/css" id="current-color-scheme">
  .ColorScheme-Background { color:#141c38; }
  .ColorScheme-Text { color:#e6ecff; }
  .ColorScheme-Highlight { color:#7c5cff; }
</style>"""

L = 10          # largo de los bordes y del centro (Plasma los estira)


def defs(sh):
    return f"""<defs>
  <linearGradient id="gloss" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#ffffff" stop-opacity=".30"/><stop offset="1" stop-color="#ffffff" stop-opacity=".10"/>
  </linearGradient>
  <linearGradient id="body" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#ffffff" stop-opacity=".08"/><stop offset=".55" stop-color="#ffffff" stop-opacity="0"/>
    <stop offset="1" stop-color="#000000" stop-opacity=".10"/>
  </linearGradient>
  <linearGradient id="bodyb" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#000000" stop-opacity="0"/><stop offset="1" stop-color="#000000" stop-opacity=".14"/>
  </linearGradient>
  <linearGradient id="lip" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#000000" stop-opacity=".30"/><stop offset="1" stop-color="#000000" stop-opacity=".55"/>
  </linearGradient>
  <linearGradient id="sh-v" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#000" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity="{sh}"/>
  </linearGradient>
  <linearGradient id="sh-vr" x1="0" y1="1" x2="0" y2="0">
    <stop offset="0" stop-color="#000" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity="{sh}"/>
  </linearGradient>
  <linearGradient id="sh-h" x1="0" y1="0" x2="1" y2="0">
    <stop offset="0" stop-color="#000" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity="{sh}"/>
  </linearGradient>
  <linearGradient id="sh-hr" x1="1" y1="0" x2="0" y2="0">
    <stop offset="0" stop-color="#000" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity="{sh}"/>
  </linearGradient>
  <radialGradient id="sh-c" cx="1" cy="1" r="1">
    <stop offset="0" stop-color="#000" stop-opacity="{sh}"/><stop offset="1" stop-color="#000" stop-opacity="0"/>
  </radialGradient>
</defs>"""


def frame(R, alpha, three_d, lip=0, ox=0, oy=0, prefix=""):
    """Las nueve piezas de un rectángulo redondeado de radio R, a partir de (ox, oy). Todo son rellenos (sin
    trazos): un trazo agranda la caja de la pieza y al estirarla se verían las juntas."""
    p = prefix + "-" if prefix else ""
    W = R + L + R
    out = []
    bg = f'class="ColorScheme-Background" fill="currentColor" fill-opacity="{alpha}"'
    light = ".42" if three_d else ".12"        # borde de arriba y de los lados
    dark = ".30" if three_d else ".12"         # borde de abajo
    dcol = "#000000" if three_d else "#ffffff"

    def piece(name, x, y, w, h, shapes):
        out.append(f'<g id="{p}{name}"><rect x="{ox + x}" y="{oy + y}" width="{w}" height="{h}" fill="none"/>{shapes}</g>')

    def quarter(cx, cy, r, which):
        """cuarto de círculo de centro (cx, cy): which = tl, tr, bl, br"""
        sx = -1 if which in ("tl", "bl") else 1
        sy = -1 if which in ("tl", "tr") else 1
        x1, y1 = cx + sx * r, cy
        x2, y2 = cx, cy + sy * r
        sweep = 1 if (which in ("tl", "br")) else 0
        return f"M{cx},{cy} L{x1},{y1} A{r},{r} 0 0 {sweep} {x2},{y2} Z"

    def ring(cx, cy, r, which, w=1):
        """el borde de un cuarto de círculo como anillo relleno de ancho w"""
        sx = -1 if which in ("tl", "bl") else 1
        sy = -1 if which in ("tl", "tr") else 1
        sweep = 1 if (which in ("tl", "br")) else 0
        ri = r - w
        return (f"M{cx + sx * r},{cy} A{r},{r} 0 0 {sweep} {cx},{cy + sy * r} L{cx},{cy + sy * ri} "
                f"A{ri},{ri} 0 0 {1 - sweep} {cx + sx * ri},{cy} Z")

    # esquinas (centro del arco en la esquina interior de la pieza)
    for name, which, x, y in (("topleft", "tl", 0, 0), ("topright", "tr", R + L, 0),
                              ("bottomleft", "bl", 0, R + L), ("bottomright", "br", R + L, R + L)):
        cx = ox + (x + R if which in ("tl", "bl") else x)
        cy = oy + (y + R if which in ("tl", "tr") else y)
        sh = ""
        if which in ("bl", "br") and lip:
            # el canto: la misma esquina un poco más abajo, más oscura
            sh += f'<path d="{quarter(cx, cy + lip, R, which)}" {bg}/><path d="{quarter(cx, cy + lip, R, which)}" fill="url(#lip)"/>'
        sh += f'<path d="{quarter(cx, cy, R, which)}" {bg}/>'
        if three_d:
            sh += f'<path d="{quarter(cx, cy, R, which)}" fill="url(#{"gloss" if which in ("tl", "tr") else "bodyb"})"/>'
        top = which in ("tl", "tr")
        sh += f'<path d="{ring(cx, cy, R, which)}" fill="{"#ffffff" if top else dcol}" fill-opacity="{light if top else dark}"/>'
        piece(name, x, y, R, R + (lip if not top else 0), sh)

    def rect(x, y, w, h, fill):
        return f'<rect x="{ox + x}" y="{oy + y}" width="{w}" height="{h}" {fill}/>'

    # bordes y centro
    piece("top", R, 0, L, R, rect(R, 0, L, R, bg) + (rect(R, 0, L, R, 'fill="url(#gloss)"') if three_d else "") +
          rect(R, 0, L, 1, f'fill="#ffffff" fill-opacity="{light}"'))
    piece("bottom", R, R + L, L, R + lip,
          (rect(R, R + L + R, L, lip, bg) + rect(R, R + L + R, L, lip, 'fill="url(#lip)"') if lip else "") +
          rect(R, R + L, L, R, bg) + (rect(R, R + L, L, R, 'fill="url(#bodyb)"') if three_d else "") +
          rect(R, R + L + R - 1, L, 1, f'fill="{dcol}" fill-opacity="{dark}"'))
    piece("left", 0, R, R, L, rect(0, R, R, L, bg) +
          rect(0, R, 1, L, f'fill="#ffffff" fill-opacity="{light}"'))
    piece("right", R + L, R, R, L, rect(R + L, R, R, L, bg) +
          rect(W - 1, R, 1, L, f'fill="#ffffff" fill-opacity="{light}"'))
    piece("center", R, R, L, L, rect(R, R, L, L, bg))
    return "\n".join(out)


def mask(R, lip, ox, oy, prefix="mask"):
    """Forma opaca para el desenfoque y la entrada del ratón."""
    W = R + L + R
    out = []
    b = 'fill="#000000"'
    shapes = {
        "topleft": (0, 0, R, R, f'<path d="M{ox},{oy + R} A{R},{R} 0 0 1 {ox + R},{oy} V{oy + R} Z" {b}/>'),
        "topright": (R + L, 0, R, R, f'<path d="M{ox + R + L},{oy} A{R},{R} 0 0 1 {ox + W},{oy + R} H{ox + R + L} Z" {b}/>'),
        "bottomleft": (0, R + L, R, R + lip, f'<path d="M{ox},{oy + R + L} H{ox + R} V{oy + R + L + R + lip} H{ox + R} A{R},{R} 0 0 1 {ox},{oy + R + L} Z" {b}/>'),
        "bottomright": (R + L, R + L, R, R + lip, f'<path d="M{ox + R + L},{oy + R + L} H{ox + W} A{R},{R} 0 0 1 {ox + R + L},{oy + R + L + R} V{oy + R + L + R + lip} Z" {b}/>'),
        "top": (R, 0, L, R, None), "bottom": (R, R + L, L, R + lip, None), "left": (0, R, R, L, None),
        "right": (R + L, R, R, L, None), "center": (R, R, L, L, None),
    }
    for name, (x, y, w, h, s) in shapes.items():
        fill = 'fill="none"' if s else b
        rect = f'<rect x="{ox + x}" y="{oy + y}" width="{w}" height="{h}" {fill}/>'
        out.append(f'<g id="{prefix}-{name}">{rect}{s or ""}</g>')
    return "\n".join(out)


def shadow(S, ox, oy, top=None):
    """Sombra suave alrededor (degradados): S de ancho; arriba, menos."""
    t = top if top is not None else S // 2
    out = []
    c = S
    parts = {
        "topleft": (0, 0, c, t, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-c)" fill-opacity=".6"/>'),
        "top": (c, 0, L, t, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-v)" fill-opacity=".6"/>'),
        "topright": (c + L, 0, c, t, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-c)" fill-opacity=".6" transform="translate(TX,0) scale(-1,1)"/>'),
        "left": (0, t, c, L, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-h)"/>'),
        "right": (c + L, t, c, L, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-hr)"/>'),
        "bottomleft": (0, t + L, c, S, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-c)" transform="translate(0,TY) scale(1,-1)"/>'),
        "bottom": (c, t + L, L, S, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-vr)"/>'),
        "bottomright": (c + L, t + L, c, S, '<rect x="X" y="Y" width="W" height="H" fill="url(#sh-c)" transform="translate(TX,TY) scale(-1,-1)"/>'),
    }
    for name, (x, y, w, h, s) in parts.items():
        X, Y = ox + x, oy + y
        s = (s.replace('x="X"', f'x="{X}"').replace('y="Y"', f'y="{Y}"').replace('width="W"', f'width="{w}"')
              .replace('height="H"', f'height="{h}"').replace("TX", str(2 * X + w)).replace("TY", str(2 * Y + h)))
        if "scale(-1,1)" in s:
            s = s.replace(f'x="{X}"', f'x="{X}"')
        out.append(f'<g id="shadow-{name}"><rect x="{X}" y="{Y}" width="{w}" height="{h}" fill="none"/>{s}</g>')
    out.append(f'<rect id="shadow-center" x="{ox + c}" y="{oy + t}" width="{L}" height="{L}" fill="none"/>')
    for side, size in (("top", t), ("bottom", S), ("left", S), ("right", S)):
        w, h = (size, size)
        out.append(f'<rect id="shadow-hint-{side}-margin" x="{ox}" y="{oy + 60}" width="{w}" height="{h}" fill="none"/>')
    return "\n".join(out)


def hints(margins, ox, oy, prefix=""):
    p = prefix + "-" if prefix else ""
    out = []
    for side, v in margins.items():
        out.append(f'<rect id="{p}hint-{side}-margin" x="{ox}" y="{oy}" width="{v}" height="{v}" fill="none"/>')
    return "\n".join(out)


def svg(body, w=400, h=300, sh=".32"):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">\n{STYLE}\n'
            f'{defs(sh)}\n{body}\n</svg>\n')


def panel(three_d, solid):
    R = 12
    lip = 6 if three_d else 0
    alpha = (1 if solid else (.80 if three_d else .72))
    body = frame(R, alpha, three_d, lip, 0, 0)
    body += "\n" + mask(R, lip, 100, 0)
    body += "\n" + shadow(14 if three_d else 10, 200, 0, 4)
    # márgenes del contenido: en 3D, más abajo por el canto
    body += "\n" + hints({"top": 5, "bottom": 5 + lip, "left": 8, "right": 8}, 0, 120)
    # el dock flotante se separa del borde de la pantalla
    body += "\n" + hints({"top": 6, "bottom": 8, "left": 8, "right": 8}, 40, 120, "floating")
    body += '\n<rect id="floating-center" x="80" y="120" width="10" height="10" fill="none"/>'
    return svg(body, sh=".42" if three_d else ".30")


def dialog(three_d, solid=False, R=12):
    alpha = 1 if solid else .92
    body = frame(R, alpha, three_d, 0, 0, 0)
    body += "\n" + mask(R, 0, 100, 0)
    body += "\n" + shadow(16, 200, 0, 8)
    body += "\n" + hints({"top": 8, "bottom": 8, "left": 8, "right": 8}, 0, 120)
    return svg(body, sh=".38")


def metadata(id_, name, desc):
    return json.dumps({
        "KPlugin": {"Authors": [{"Name": "RROS"}], "Description": desc, "Id": id_, "License": "GPL-2.0+",
                    "Name": name, "Version": "1.0", "Website": "https://github.com/raulramalf/RROS"},
        "X-Plasma-API-Minimum-Version": "6.0",
    }, ensure_ascii=False, indent=2) + "\n"


def main():
    for id_, name, three_d, desc in (("rros", "RROS", False, "Barra y dock planos y translúcidos (2D)"),
                                     ("rros-3d", "RROS 3D", True, "Barra y dock con brillo, bisel y canto, como un estante de cristal (3D)")):
        d = os.path.join(OUT, id_)
        for sub in ("widgets", "dialogs", "solid/widgets", "solid/dialogs"):
            os.makedirs(os.path.join(d, sub), exist_ok=True)
        with open(os.path.join(d, "metadata.json"), "w") as f:
            f.write(metadata(id_, name, desc))
        files = {
            "widgets/panel-background.svg": panel(three_d, False),
            "solid/widgets/panel-background.svg": panel(three_d, True),
            "dialogs/background.svg": dialog(False),
            "solid/dialogs/background.svg": dialog(False, True),
            "widgets/tooltip.svg": dialog(False, False, 10),
            "solid/widgets/tooltip.svg": dialog(False, True, 10),
        }
        for rel, text in files.items():
            with open(os.path.join(d, rel), "w") as f:
                f.write(text)


if __name__ == "__main__":
    main()
