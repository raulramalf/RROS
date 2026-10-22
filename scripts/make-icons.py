#!/usr/bin/env python3
"""Iconos propios de Lily (SVG): las aplicaciones del sistema y las secciones de Lily Ajustes.
El logotipo (rros, rros-symbolic y app-rros) sale de docs/brand/rros-logo.svg con make-logo.py.

Todas siguen el mismo dibujo: un cuadrado redondeado con un degradado, un brillo arriba y el símbolo en blanco.
Las aplicaciones que no son de Lily (Firefox, Dolphin...) usan su propio logotipo: aquí no se dibujan.

uso: make-icons.py  (escribe en overlay/arch/usr/share/icons/hicolor/scalable/apps y en app/icons)
"""
import math
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
HICOLOR = os.path.join(ROOT, "overlay/arch/usr/share/icons/hicolor/scalable/apps")
APPICONS = os.path.join(ROOT, "app/icons")

S = 'stroke="#fff" stroke-width="7" stroke-linecap="round" stroke-linejoin="round" fill="none"'
W = 'fill="#fff"'


def gear(cx=50, cy=50, r1=27, r2=20, teeth=8, hole=9):
    pts = []
    for i in range(teeth * 4):
        a = (i / (teeth * 4)) * 2 * math.pi - math.pi / 2
        r = r1 if (i % 4) in (1, 2) else r2
        pts.append(f"{cx + r * math.cos(a):.1f},{cy + r * math.sin(a):.1f}")
    ring = (f'M{cx + hole},{cy} a{hole},{hole} 0 1,0 {-2 * hole},0 a{hole},{hole} 0 1,0 {2 * hole},0z')
    return f'<path fill-rule="evenodd" {W} d="M{" L".join(pts)}Z {ring}"/>'


GLYPHS = {
    # --- aplicaciones de Lily ---
    "ajustes": gear(),
    "tienda": f'<path {W} d="M28 42h44l-3.5 31a5 5 0 0 1-5 4.4H36.5a5 5 0 0 1-5-4.4z"/>'
              f'<path {S} d="M39 44V36a11 11 0 0 1 22 0v8"/>',
    "instalar": f'<path {S} d="M50 22v34M36 44l14 14 14-14"/><path {S} d="M27 64v8h46v-8"/>',
    "bienvenida": f'<path {W} d="M50 20c2.6 15.5 9.5 25 30 30-20.5 5-27.4 14.5-30 30-2.6-15.5-9.5-25-30-30 20.5-5 27.4-14.5 30-30z"/>'
                  f'<circle cx="74" cy="26" r="5" fill="#fff" opacity=".85"/>',
    "launchpad": "".join(f'<rect x="{25 + c * 18}" y="{25 + r * 18}" width="13" height="13" rx="3.5" fill="#fff" '
                         f'opacity="{1 - (r + c) * 0.08:.2f}"/>' for r in range(3) for c in range(3)),
    "seguridad": f'<path {W} d="M50 20l26 9v19c0 15-10.6 26.4-26 32-15.4-5.6-26-17-26-32V29z"/>'
                 f'<path stroke="#0e7a55" stroke-width="7" stroke-linecap="round" stroke-linejoin="round" fill="none" '
                 f'd="M38 50l8 8 16-16"/>',
    "dispositivos": f'<rect x="24" y="32" width="52" height="36" rx="8" {S}/><circle cx="64" cy="58" r="4.5" fill="#fff"/>'
                    f'<path {S} d="M33 45h22"/>',
    "nuevo": f'<path {W} d="M32 20h24l16 16v40a4 4 0 0 1-4 4H32a4 4 0 0 1-4-4V24a4 4 0 0 1 4-4z" opacity=".95"/>'
             f'<path stroke="#2563eb" stroke-width="6.5" stroke-linecap="round" fill="none" d="M50 46v20M40 56h20"/>',
    "actualizar": f'<path {S} d="M71 44a22 22 0 0 0-39-9M29 56a22 22 0 0 0 39 9"/><path {W} d="M33 22v15h15z"/>'
                  f'<path {W} d="M67 78V63H52z"/>',
    "monitor": f'<rect x="22" y="26" width="56" height="38" rx="6" {S}/><path {S} d="M40 76h20M50 64v12"/>',
    "generica": f'<rect x="28" y="28" width="44" height="44" rx="10" {S}/>',
    # --- secciones de Ajustes ---
    "general": f'<circle cx="50" cy="30" r="5.5" fill="#fff"/><path {S} d="M50 44v30"/>',
    "apariencia": f'<path {W} d="M50 22a28 28 0 1 0 0 56c4 0 6-2.6 6-5.6 0-4.6-4-6-4-10 0-3.4 2.6-5.4 6-5.4h6c8 0 14-5 14-13C78 31 66 22 50 22z"/>'
                  f'<circle cx="36" cy="44" r="5" fill="#8b5cf6"/><circle cx="48" cy="33" r="5" fill="#f43f5e"/>'
                  f'<circle cx="62" cy="36" r="5" fill="#f59e0b"/><circle cx="37" cy="59" r="5" fill="#10b981"/>',
    "escritorio": f'<rect x="20" y="24" width="60" height="40" rx="6" {S}/><rect x="31" y="70" width="38" height="8" rx="4" fill="#fff"/>',
    "red": f'<path {S} d="M22 42a40 40 0 0 1 56 0M31 52a27 27 0 0 1 38 0M40 62a14 14 0 0 1 20 0"/><circle cx="50" cy="72" r="5" fill="#fff"/>',
    "bluetooth": f'<path {S} d="M36 36l28 28-14 12V24l14 12-28 28"/>',
    "sonido": f'<path {W} d="M24 42h12l15-13v42L36 58H24z"/><path {S} d="M61 40a14 14 0 0 1 0 20M68 32a25 25 0 0 1 0 36"/>',
    "pantalla": f'<circle cx="50" cy="50" r="11" fill="#fff"/>'
                + "".join(f'<path {S} d="M{50 + 21 * math.cos(a):.1f} {50 + 21 * math.sin(a):.1f}L{50 + 28 * math.cos(a):.1f} '
                          f'{50 + 28 * math.sin(a):.1f}"/>' for a in [i * math.pi / 4 for i in range(8)]),
    "energia": f'<path {W} d="M55 18L28 56h18l-4 26 28-40H52z"/>',
    "raton": f'<rect x="34" y="20" width="32" height="58" rx="16" {S}/><path {S} d="M50 30v10"/>',
    "teclado": f'<rect x="18" y="32" width="64" height="38" rx="7" {S}/>'
               + "".join(f'<rect x="{27 + c * 10}" y="{41 + r * 9}" width="6" height="5" rx="1.5" fill="#fff"/>'
                         for r in range(2) for c in range(5)) + f'<rect x="34" y="59" width="32" height="5" rx="2" fill="#fff"/>',
    "accesibilidad": f'<circle cx="50" cy="26" r="6" fill="#fff"/><path {S} d="M27 38l23 5 23-5M50 43v14l-10 18M50 57l10 18"/>',
    "avanzado": f'<path {S} d="M28 30h44M28 50h44M28 70h44"/><circle cx="40" cy="30" r="6" fill="#fff"/>'
                f'<circle cx="60" cy="50" r="6" fill="#fff"/><circle cx="46" cy="70" r="6" fill="#fff"/>',
    "fecha": f'<circle cx="50" cy="50" r="27" {S}/><path {S} d="M50 34v17l11 7"/>',
    "notificaciones": f'<path {W} d="M50 22c-11 0-19 8-19 19v13l-6 9h50l-6-9V41c0-11-8-19-19-19z"/><path {S} d="M43 72a7 7 0 0 0 14 0"/>',
    "usuarios": f'<circle cx="50" cy="38" r="12" fill="#fff"/><path {W} d="M26 76c2-13 11-20 24-20s22 7 24 20z"/>',
}

# degradados (arriba, abajo) de cada icono
COLORS = {  # paleta de las secciones: tonos de la marca Lirio (vino, rojo lirio, champán) y fríos apagados
    "monitor": ("#8f8ae0", "#4f3fa8"), "generica": ("#a49a9c", "#5f5457"),
    "general": ("#a89ea0", "#625659"), "apariencia": ("#b56ac6", "#6a2a85"), "escritorio": ("#5b8fe0", "#2a4fa6"),
    "red": ("#4a9ae8", "#1d5bb8"), "bluetooth": ("#5a86e6", "#2846a8"), "sonido": ("#ec5a72", "#a8183a"),
    "pantalla": ("#8a7ae6", "#4a35a6"), "energia": ("#3fb88c", "#13735a"), "raton": ("#a89ea0", "#625659"),
    "teclado": ("#a89ea0", "#625659"), "accesibilidad": ("#4a9ae8", "#1d5bb8"), "avanzado": ("#8a7d80", "#463a3e"),
    "fecha": ("#f0b880", "#c06c34"), "notificaciones": ("#e8475f", "#9e1530"), "usuarios": ("#d9a37a", "#9a5a36"),
}


def squircle(name, glyph, shadow=True):
    """Icono de sección: losa pequeña con degradado de dos tonos, borde de luz y símbolo blanco (sin filtros)."""
    top, bottom = COLORS[name]
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="100" height="100">
  <defs>
    <linearGradient id="g" x1="0" y1="0" x2=".35" y2="1"><stop offset="0" stop-color="{top}"/><stop offset="1" stop-color="{bottom}"/></linearGradient>
    <linearGradient id="h" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".28"/><stop offset=".55" stop-color="#fff" stop-opacity="0"/></linearGradient>
    <linearGradient id="r" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".45"/><stop offset=".5" stop-color="#fff" stop-opacity=".08"/><stop offset="1" stop-color="#000" stop-opacity=".22"/></linearGradient>
  </defs>
  <rect x="6" y="6" width="88" height="88" rx="22" fill="url(#g)"/>
  <rect x="6" y="6" width="88" height="88" rx="22" fill="url(#h)"/>
  <rect x="6.75" y="6.75" width="86.5" height="86.5" rx="21.25" fill="none" stroke="url(#r)" stroke-width="1.5"/>
  <g transform="translate(0 1.6)" opacity=".22">{glyph.replace('#fff', '#000')}</g>
  {glyph}
</svg>
'''


def lgrad(gid, stops, x1=0, y1=0, x2=0, y2=1):
    s = "".join(f'<stop offset="{o}" stop-color="{c}"' + (f' stop-opacity="{a}"' if a != 1 else "") + "/>"
                for o, c, a in [(t + (1,))[:3] for t in stops])
    return f'<linearGradient id="{gid}" x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}">{s}</linearGradient>'


def rgrad(gid, stops, cx=.5, cy=.5, r=.5):
    s = "".join(f'<stop offset="{o}" stop-color="{c}"' + (f' stop-opacity="{a}"' if a != 1 else "") + "/>"
                for o, c, a in [(t + (1,))[:3] for t in stops])
    return f'<radialGradient id="{gid}" cx="{cx}" cy="{cy}" r="{r}">{s}</radialGradient>'


def lirio(cx, cy, s, petal="#fff", stamen="#fff", op=1):
    """Lirio araña estilizado: seis pétalos curvos y estambres finos (escala s = radio)."""
    out = []
    for i in range(6):
        a = i * 60 - 90
        out.append(f'<path transform="translate({cx} {cy}) rotate({a}) scale({s / 10:.3f})" fill="{petal}" opacity="{op}" '
                   f'd="M0 0C2.5-1.6 6.5-2.6 10-1.2 8.6-.2 7.5 1.2 6.4 1.4 4 1.6 1.8 1.1 0 0z"/>')
    for i in range(6):
        a = math.radians(i * 60 - 60)
        x2, y2 = cx + s * 1.25 * math.cos(a), cy + s * 1.25 * math.sin(a)
        out.append(f'<path d="M{cx} {cy}Q{cx + s * .5 * math.cos(a - .5):.1f} {cy + s * .5 * math.sin(a - .5):.1f} {x2:.1f} {y2:.1f}" '
                   f'stroke="{stamen}" stroke-width="{s * .07:.2f}" fill="none" stroke-linecap="round" opacity="{op}"/>'
                   f'<circle cx="{x2:.1f}" cy="{y2:.1f}" r="{s * .1:.2f}" fill="{stamen}" opacity="{op}"/>')
    return "".join(out)


def gear_d(cx=50, cy=50, r1=27, r2=20, teeth=8, hole=9):
    pts = []
    for i in range(teeth * 4):
        a = (i / (teeth * 4)) * 2 * math.pi - math.pi / 2
        r = r1 if (i % 4) in (1, 2) else r2
        pts.append(f"{cx + r * math.cos(a):.1f},{cy + r * math.sin(a):.1f}")
    ring = f"M{cx + hole},{cy} a{hole},{hole} 0 1,0 {-2 * hole},0 a{hole},{hole} 0 1,0 {2 * hole},0z" if hole else ""
    return f'M{" L".join(pts)}Z {ring}'


def sombra(d, dy=2.5, op=.32):
    """Sombra de un objeto: la misma silueta, desplazada y translúcida (QtSvg no admite filtros)."""
    return (f'<path d="{d}" fill="#000" opacity="{op * .5:.2f}" transform="translate(0 {dy * 1.6:.1f})"/>'
            f'<path d="{d}" fill="#000" opacity="{op:.2f}" transform="translate(0 {dy})"/>')


# --- aplicaciones: cuerpo de losa con volumen + ilustración de varias capas ---
# cuerpo (claro, medio, oscuro)
BODY = {
    "ajustes": ("#4a2430", "#2a1018", "#140609"),
    "tienda": ("#fffaf2", "#f3e4cf", "#d9b98f"),
    "instalar": ("#34416a", "#1f2745", "#10142a"),
    "bienvenida": ("#ef4a63", "#c8243c", "#6e0c1d"),
    "launchpad": ("#3a1a24", "#220d13", "#120508"),
    "seguridad": ("#2f8a76", "#17594c", "#0a2f29"),
    "dispositivos": ("#f6d7ae", "#e3ab72", "#a9653a"),
    "nuevo": ("#5578b0", "#2f4c80", "#18284a"),
    "actualizar": ("#8a4a9a", "#5a2768", "#2c1033"),
}


def lienzo(name, art, extra_defs=""):
    a, b, c = BODY[name]
    claro = name in ("tienda", "dispositivos")
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="100" height="100">
  <defs>
    {rgrad("cs", [(0, "#000", .42), (.6, "#000", .12), (1, "#000", 0)])}
    {lgrad("bd", [(0, a), (.55, b), (1, c)], 0, 0, .3, 1)}
    {rgrad("gl", [(0, "#fff", .32 if not claro else .7), (1, "#fff", 0)], .3, .1, .75)}
    {lgrad("rim", [(0, "#fff", .55 if not claro else .9), (.35, "#fff", .1), (.8, "#000", .05), (1, "#000", .3)])}
    {lgrad("fl", [(0, "#000", 0), (1, "#000", .16)])}
    {extra_defs}
  </defs>
  <ellipse cx="50" cy="94" rx="40" ry="4.5" fill="url(#cs)"/>
  <rect x="7" y="8" width="86" height="86" rx="21.5" fill="#000" opacity=".14"/>
  <g transform="translate(50 50) scale(.977) translate(-50 -49)">
  <rect x="6" y="5" width="88" height="88" rx="22" fill="url(#bd)"/>
  <rect x="6" y="5" width="88" height="88" rx="22" fill="url(#gl)"/>
  <rect x="6" y="49" width="88" height="44" rx="22" fill="url(#fl)"/>
  <rect x="6.75" y="5.75" width="86.5" height="86.5" rx="21.25" fill="none" stroke="url(#rim)" stroke-width="1.5"/>
  {art}
  </g>
</svg>
'''


def art_ajustes():
    g = gear_d(50, 49, 30, 22.5, 9, 0)
    d = lgrad("m", [(0, "#fbf3e6"), (.45, "#e6c9a0"), (.7, "#b88d5e"), (1, "#7d5634")], 0, 0, .4, 1)
    d += lgrad("mi", [(0, "#8a6240"), (1, "#f3ddbd")], 0, 0, 0, 1)
    d += rgrad("hub", [(0, "#fff8ee"), (.6, "#e2c49b"), (1, "#a37a4e")], .38, .32, .7)
    d += rgrad("eje", [(0, "#c8243c"), (.7, "#7a0f22"), (1, "#3a0610")], .4, .35, .6)
    art = (sombra(g, 2.2, .45) +
           f'<path d="{g}" fill="url(#m)"/>'
           f'<path d="{g}" fill="none" stroke="#fff" stroke-opacity=".55" stroke-width=".8" transform="translate(0 .5)"/>'
           f'<circle cx="50" cy="49" r="17" fill="url(#mi)"/>'
           f'<circle cx="50" cy="49" r="15" fill="url(#m)"/>'
           + "".join(f'<circle cx="{50 + 10.5 * math.cos(math.radians(k * 60 + 30)):.1f}" cy="{49 + 10.5 * math.sin(math.radians(k * 60 + 30)):.1f}" '
                     f'r="2.1" fill="#5e3d24" opacity=".55"/>' for k in range(6)) +
           f'<circle cx="50" cy="49" r="7.5" fill="url(#hub)"/>'
           f'<circle cx="50" cy="49" r="3.6" fill="url(#eje)"/>'
           f'<ellipse cx="44" cy="35" rx="9" ry="3" fill="#fff" opacity=".35" transform="rotate(-25 44 35)"/>')
    return art, d


def art_tienda():
    d = lgrad("bag", [(0, "#ea4760"), (.55, "#c8243c"), (1, "#7d1124")], 0, 0, .25, 1)
    d += lgrad("lado", [(0, "#9c1a2e"), (1, "#5c0a19")])
    d += lgrad("asa", [(0, "#fff3e2"), (.5, "#efc9a6"), (1, "#a8784a")], 0, 0, 1, 0)
    bolsa = "M25 39h50l-3.5 39a5 5 0 0 1-5 4.5H33.5a5 5 0 0 1-5-4.5z"
    art = (f'<ellipse cx="50" cy="84" rx="27" ry="3.4" fill="#5a2a14" opacity=".22"/>' +
           f'<path d="M39 44V33a11 11 0 0 1 22 0v11" fill="none" stroke="#7a4f2b" stroke-width="5.4" stroke-linecap="round" opacity=".55"/>'
           f'<path d="{bolsa}" fill="url(#bag)"/>'
           f'<path d="M25 39h50l-.4 4.5H25.4z" fill="#000" opacity=".18"/>'
           f'<path d="M71.5 39H75l-3.5 39a5 5 0 0 1-3 4z" fill="url(#lado)" opacity=".6"/>'
           f'<path d="M27.5 41.5h3l-2 33" stroke="#fff" stroke-opacity=".35" stroke-width="1.4" fill="none" stroke-linecap="round"/>'
           f'<circle cx="39" cy="45" r="2.6" fill="#4a0712" opacity=".7"/><circle cx="61" cy="45" r="2.6" fill="#4a0712" opacity=".7"/>'
           f'<path d="M39 45V33a11 11 0 0 1 22 0v12" fill="none" stroke="url(#asa)" stroke-width="4.2" stroke-linecap="round"/>'
           + lirio(50, 63, 9.5, "#f6ecdf", "#efc9a6") +
           f'<circle cx="50" cy="63" r="1.8" fill="#efc9a6"/>')
    return art, d


def art_instalar():
    d = lgrad("caja", [(0, "#f4ecdf"), (.5, "#cfc4b6"), (1, "#8d8478")])
    d += lgrad("cara", [(0, "#3a3340"), (1, "#16121a")])
    d += lgrad("fx", [(0, "#ff5a72"), (1, "#a3142c")])
    d += rgrad("led", [(0, "#ffd0d6"), (.4, "#ff3b57"), (1, "#c8243c", 0)])
    caja = "M20 58h60a6 6 0 0 1 6 6v10a6 6 0 0 1-6 6H20a6 6 0 0 1-6-6V64a6 6 0 0 1 6-6z"
    flecha = "M43 16h14v22h11L50 56 32 38h11z"
    art = (sombra(caja, 2, .4) + f'<path d="{caja}" fill="url(#caja)"/>'
           f'<rect x="19" y="68" width="62" height="8" rx="3" fill="url(#cara)"/>'
           + "".join(f'<rect x="{23 + i * 5}" y="70.5" width="2.4" height="3" rx="1" fill="#6b6370"/>' for i in range(6)) +
           f'<circle cx="75" cy="72" r="4" fill="url(#led)"/><circle cx="75" cy="72" r="1.5" fill="#ffe3e7"/>'
           f'<path d="M20 59.5h60" stroke="#fff" stroke-opacity=".8" stroke-width="1"/>' +
           sombra(flecha, 2, .35) + f'<path d="{flecha}" fill="url(#fx)"/>'
           f'<path d="M44.5 17.5h11v22" stroke="#fff" stroke-opacity=".45" stroke-width="1.2" fill="none"/>')
    return art, d


def art_bienvenida():
    d = rgrad("pet", [(0, "#ffffff"), (.6, "#f6ecdf"), (1, "#d9b98f")], .2, .5, .9)
    d += rgrad("halo", [(0, "#ffd9a8", .55), (1, "#ffd9a8", 0)])
    out = '<circle cx="50" cy="50" r="34" fill="url(#halo)"/>'
    # pétalos largos y rizados del lirio araña, con sombra
    for capa, (col, op, dy) in enumerate([("#3a0410", .35, 2.4), ("url(#pet)", 1, 0)]):
        for i in range(6):
            a = i * 60 - 90 + 30
            out += (f'<path transform="translate(50 {51 + dy}) rotate({a}) scale(3.1)" fill="{col}" opacity="{op}" '
                    f'd="M0 0C2.5-1.8 6.5-2.8 10-1.2 8.6-.2 7.5 1.4 6.2 1.6 4 1.8 1.8 1.2 0 0z"/>')
    for i in range(6):
        a = math.radians(i * 60 - 60)
        x2, y2 = 50 + 30 * math.cos(a), 51 + 30 * math.sin(a)
        qx, qy = 50 + 14 * math.cos(a - .6), 51 + 14 * math.sin(a - .6)
        out += (f'<path d="M50 51Q{qx:.1f} {qy:.1f} {x2:.1f} {y2:.1f}" stroke="#efc9a6" stroke-width="1.6" fill="none" stroke-linecap="round"/>'
                f'<ellipse cx="{x2:.1f}" cy="{y2:.1f}" rx="2.6" ry="1.6" fill="#ffe2b8" transform="rotate({i * 60 - 60} {x2:.1f} {y2:.1f})"/>')
    out += '<circle cx="50" cy="51" r="4.5" fill="#efc9a6"/><circle cx="49" cy="50" r="1.8" fill="#fff"/>'
    return out, d


def art_launchpad():
    cols = [("#ef4a63", "#a3142c"), ("#f3d3ad", "#c8955f"), ("#fff7ec", "#d9c5ab"),
            ("#3fa38e", "#155a4c"), ("#ef4a63", "#a3142c"), ("#9a5aab", "#4a1f57"),
            ("#5578b0", "#1f3563"), ("#f3d3ad", "#c8955f"), ("#ef4a63", "#a3142c")]
    d = "".join(lgrad(f"t{i}", [(0, a), (1, b)], 0, 0, .3, 1) for i, (a, b) in enumerate(cols))
    d += lgrad("bri", [(0, "#fff", .6), (1, "#fff", 0)])
    out = ""
    for i in range(9):
        r, c = divmod(i, 3)
        x, y = 21 + c * 21, 20 + r * 21
        out += (f'<rect x="{x}" y="{y + 1.6}" width="16" height="16" rx="5" fill="#000" opacity=".38"/>'
                f'<rect x="{x}" y="{y}" width="16" height="16" rx="5" fill="url(#t{i})"/>'
                f'<rect x="{x + 1.5}" y="{y + 1}" width="13" height="7" rx="3.5" fill="url(#bri)"/>')
    return out, d


def art_seguridad():
    d = lgrad("oro", [(0, "#fff2dc"), (.4, "#efc9a6"), (.75, "#c4945f"), (1, "#7d5634")], 0, 0, .5, 1)
    d += lgrad("vino", [(0, "#d0304a"), (.6, "#8e1428"), (1, "#3d0611")], 0, 0, .3, 1)
    d += lgrad("ojo", [(0, "#fffaf2"), (1, "#e3cdb0")])
    esc = "M50 15l29 10v21c0 18-12 30.5-29 37-17-6.5-29-19-29-37V25z"
    inn = "M50 22.5l22 7.6v16.2c0 14-9 23.8-22 29-13-5.2-22-15-22-29V30.1z"
    art = (sombra(esc, 2.4, .45) + f'<path d="{esc}" fill="url(#oro)"/>'
           f'<path d="{inn}" fill="url(#vino)"/>'
           f'<path d="M50 22.5l22 7.6v16.2c0 1-.05 2-.15 3H28.15c-.1-1-.15-2-.15-3V30.1z" fill="#fff" opacity=".1"/>'
           f'<path d="M50 16.5l27.5 9.5" stroke="#fff" stroke-opacity=".8" stroke-width="1" fill="none"/>'
           f'<circle cx="50" cy="44" r="7" fill="url(#ojo)"/><path d="M46.5 47h7l2 15h-11z" fill="url(#ojo)"/>'
           f'<circle cx="50" cy="44" r="2.6" fill="#5c0a19"/><path d="M49 45h2l.8 11h-3.6z" fill="#5c0a19"/>')
    return art, d


def art_dispositivos():
    d = lgrad("cu", [(0, "#ea4760"), (.55, "#c8243c"), (1, "#6e0c1d")], 0, 0, 1, .2)
    d += lgrad("me", [(0, "#ffffff"), (.5, "#cfc8bf"), (1, "#8a837a")], 0, 0, 1, 0)
    cuerpo = "M30 40h40a8 8 0 0 1 8 8v22a8 8 0 0 1-8 8H30a8 8 0 0 1-8-8V48a8 8 0 0 1 8-8z"
    art = ('<g transform="rotate(-35 50 55)">' + sombra(cuerpo, 2.5, .3) +
           f'<rect x="78" y="47" width="15" height="24" rx="2" fill="#5a3a20" opacity=".3" transform="translate(0 2.5)"/>'
           f'<rect x="76" y="47" width="16" height="24" rx="2" fill="url(#me)"/>'
           f'<rect x="83" y="51.5" width="5" height="4" rx=".8" fill="#4a4540"/><rect x="83" y="62.5" width="5" height="4" rx=".8" fill="#4a4540"/>'
           f'<path d="{cuerpo}" fill="url(#cu)"/>'
           f'<path d="M30 42h40a6 6 0 0 1 6 6v2H24v-2a6 6 0 0 1 6-6z" fill="#fff" opacity=".25"/>'
           f'<circle cx="32" cy="59" r="3" fill="#3d0611" opacity=".6"/>'
           + lirio(52, 59, 7, "#f6ecdf", "#efc9a6") + '</g>')
    return art, d


def art_nuevo():
    d = lgrad("pap", [(0, "#fffdf8"), (1, "#eadcc7")])
    d += lgrad("dob", [(0, "#d9b98f"), (1, "#fff6e8")], 0, 1, 1, 0)
    d += lgrad("bad", [(0, "#ef4a63"), (1, "#8e1428")])
    hoja = "M30 15h27l17 17v46a5 5 0 0 1-5 5H30a5 5 0 0 1-5-5V20a5 5 0 0 1 5-5z"
    art = (sombra(hoja, 2.2, .4) + f'<path d="{hoja}" fill="url(#pap)"/>'
           f'<path d="M57 15v12a5 5 0 0 0 5 5h12z" fill="url(#dob)"/>'
           f'<path d="M57 32h17l-17 7z" fill="#000" opacity=".06"/>'
           + "".join(f'<rect x="33" y="{40 + k * 7}" width="{30 - k * 7 if k else 34}" height="2.6" rx="1.3" fill="#2f4c80" opacity=".28"/>' for k in range(3)) +
           f'<circle cx="67" cy="71" r="15.5" fill="#000" opacity=".3" transform="translate(0 2)"/>'
           f'<circle cx="67" cy="71" r="15" fill="url(#bad)"/>'
           f'<path d="M57 66a12 9 0 0 1 20 0" fill="#fff" opacity=".22"/>'
           f'<path d="M67 63v16M59 71h16" stroke="#fff" stroke-width="4.2" stroke-linecap="round"/>')
    return art, d


def art_actualizar():
    d = lgrad("fa", [(0, "#fff6e8"), (.5, "#efc9a6"), (1, "#b38052")], 0, 0, 1, 1)
    d += rgrad("pu", [(0, "#ffb3bf"), (.45, "#e33a55"), (1, "#7d1124")], .38, .35, .65)

    def arco(a0, a1, r=24):
        p = lambda a: (50 + r * math.cos(math.radians(a)), 49 + r * math.sin(math.radians(a)))
        (x0, y0), (x1, y1) = p(a0), p(a1)
        return f"M{x0:.1f} {y0:.1f}A{r} {r} 0 0 1 {x1:.1f} {y1:.1f}"

    def punta(a, r=24):
        rad = math.radians(a)
        cx, cy = 50 + r * math.cos(rad), 49 + r * math.sin(rad)
        tx, ty = -math.sin(rad), math.cos(rad)  # tangente (sentido horario)
        nx, ny = math.cos(rad), math.sin(rad)
        pts = [(cx + tx * 9, cy + ty * 9), (cx + nx * 8, cy + ny * 8), (cx - nx * 8, cy - ny * 8)]
        return "M" + " L".join(f"{x:.1f},{y:.1f}" for x, y in pts) + "z"

    flechas = ""
    for a0 in (200, 20):
        flechas += f'<path d="{arco(a0, a0 + 125)}" stroke="#%s" stroke-width="8" stroke-linecap="round" fill="none"/>' \
                   f'<path d="{punta(a0 + 125)}" fill="#%s" stroke="#%s" stroke-width="1.5" stroke-linejoin="round"/>'
    som = flechas.replace("#%s", "#000")
    luz = flechas.replace("#%s", "url(#fa)").replace("fill=\"url(#fa)\" stroke=\"url(#fa)\"", "fill=\"url(#fa)\" stroke=\"none\"")
    luz = luz.replace("url(#fa)", "§").replace("§", "url(#fa)")
    art = (f'<g opacity=".4" transform="translate(0 2.4)">{som}</g>{luz}'
           f'<circle cx="50" cy="51" r="9" fill="#000" opacity=".3"/>'
           f'<circle cx="50" cy="49" r="9" fill="url(#pu)"/><ellipse cx="47.5" cy="45.5" rx="3.6" ry="2.2" fill="#fff" opacity=".55"/>')
    return art, d


ARTE = {"ajustes": art_ajustes, "tienda": art_tienda, "instalar": art_instalar, "bienvenida": art_bienvenida,
        "launchpad": art_launchpad, "seguridad": art_seguridad, "dispositivos": art_dispositivos,
        "nuevo": art_nuevo, "actualizar": art_actualizar}


def app_icon(name):
    art, d = ARTE[name]()
    return lienzo(name, art, d)


APPS = {  # nombre del icono en el sistema -> dibujo
    "rros-settings": "ajustes", "rros-apps": "tienda", "rros-install": "instalar",
    "rros-welcome": "bienvenida", "rros-launchpad": "launchpad", "rros-security": "seguridad",
    "rros-devices": "dispositivos", "rros-new": "nuevo", "rros-updates": "actualizar",
}


def main():
    os.makedirs(HICOLOR, exist_ok=True)
    os.makedirs(APPICONS, exist_ok=True)
    for icon, g in APPS.items():
        with open(os.path.join(HICOLOR, icon + ".svg"), "w") as f:
            f.write(app_icon(g))
    for g in GLYPHS:
        with open(os.path.join(APPICONS, ("app-" if g in APPS.values() else "sec-") + g + ".svg"), "w") as f:
            f.write(app_icon(g) if g in APPS.values() else squircle(g, GLYPHS[g]))


if __name__ == "__main__":
    main()
