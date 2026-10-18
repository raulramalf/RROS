#!/usr/bin/env python3
"""Iconos propios de RROS (SVG): el logotipo, las aplicaciones del sistema y las secciones de RROS Ajustes.

Todas siguen el mismo dibujo: un cuadrado redondeado con un degradado, un brillo arriba y el símbolo en blanco.
Las aplicaciones que no son de RROS (Firefox, Dolphin...) usan su propio logotipo: aquí no se dibujan.

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


def rros_mark(color="#fff", extra=""):
    # una «R» geométrica con la pata en curva, como una aurora
    return (f'<path fill="{color}" {extra} d="M30 76V24h21c11 0 18 6.4 18 15.6 0 7-3.8 12-10.2 14.2L71 76H59.6L49.4 '
            f'55.4H40.2V76z M40.2 46.8h10.2c5.1 0 8.1-2.6 8.1-6.9s-3-6.9-8.1-6.9H40.2z"/>')


GLYPHS = {
    # --- aplicaciones de RROS ---
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
    "rros": rros_mark(),
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
COLORS = {
    "ajustes": ("#4c5675", "#1d2236"), "tienda": ("#8b6dff", "#4f7dff"), "instalar": ("#2fd6f0", "#3b6cf6"),
    "bienvenida": ("#ff7ab6", "#7c5cff"), "launchpad": ("#3b4256", "#141824"), "seguridad": ("#3ee0a4", "#0b8f63"),
    "dispositivos": ("#ffc53d", "#f9731a"), "nuevo": ("#6db1ff", "#2b67e8"), "actualizar": ("#5ee2a0", "#139a74"),
    "monitor": ("#9aa4ff", "#5b4de0"), "generica": ("#9aa3b5", "#5d6578"), "rros": ("#7c5cff", "#16c6e6"),
    "general": ("#a1a7b6", "#636a7c"), "apariencia": ("#c17bff", "#7c3aed"), "escritorio": ("#4da3ff", "#1d5fe0"),
    "red": ("#3b9bff", "#0a5fe0"), "bluetooth": ("#4b8dff", "#1f4fd8"), "sonido": ("#ff6584", "#e11d48"),
    "pantalla": ("#7d7bff", "#4338ca"), "energia": ("#4be08a", "#15a34a"), "raton": ("#a3a9b8", "#5e6577"),
    "teclado": ("#a3a9b8", "#5e6577"), "accesibilidad": ("#43a8ff", "#1666e0"), "avanzado": ("#8e95a6", "#4a5163"),
    "fecha": ("#ffab4a", "#f0640f"), "notificaciones": ("#ff6a6a", "#e02b2b"), "usuarios": ("#6aa6ff", "#3a5fe0"),
}


def squircle(name, glyph, shadow=True):
    top, bottom = COLORS[name]
    sh = ('<filter id="s" x="-20%" y="-20%" width="140%" height="150%"><feDropShadow dx="0" dy="2.5" stdDeviation="2.4" '
          'flood-color="#000" flood-opacity=".28"/></filter>') if shadow else ""
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="100" height="100">
  <defs>
    <linearGradient id="g" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="{top}"/><stop offset="1" stop-color="{bottom}"/></linearGradient>
    <linearGradient id="h" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#fff" stop-opacity=".32"/><stop offset=".5" stop-color="#fff" stop-opacity="0"/></linearGradient>
    <filter id="gs" x="-20%" y="-20%" width="140%" height="140%"><feDropShadow dx="0" dy="1.2" stdDeviation="1.2" flood-color="#000" flood-opacity=".22"/></filter>
    {sh}
  </defs>
  <g{' filter="url(#s)"' if shadow else ''}>
    <rect x="6" y="5" width="88" height="88" rx="21" fill="url(#g)"/>
    <rect x="6" y="5" width="88" height="88" rx="21" fill="url(#h)"/>
    <rect x="6.5" y="5.5" width="87" height="87" rx="20.5" fill="none" stroke="#fff" stroke-opacity=".18"/>
  </g>
  <g filter="url(#gs)">{glyph}</g>
</svg>
'''


def symbolic_mark():
    # el logotipo para la barra superior: una sola tinta que Plasma pinta del color del texto
    return '''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100" width="16" height="16">
  <style id="current-color-scheme">.ColorScheme-Text { color:#e6ecff; }</style>
  <path class="ColorScheme-Text" fill="currentColor" fill-rule="evenodd" d="M24 4h52a20 20 0 0 1 20 20v52a20 20 0 0 1-20 20H24A20 20 0 0 1 4 76V24A20 20 0 0 1 24 4z
     M30 76h10.2V55.4h9.2L59.6 76H71L58.8 53.8C65.2 51.6 69 46.6 69 39.6 69 30.4 62 24 51 24H30z M40.2 46.8V33h10.2c5.1 0 8.1 2.6 8.1 6.9s-3 6.9-8.1 6.9z"/>
</svg>
'''


APPS = {  # nombre del icono en el sistema -> dibujo
    "rros": "rros", "rros-settings": "ajustes", "rros-apps": "tienda", "rros-install": "instalar",
    "rros-welcome": "bienvenida", "rros-launchpad": "launchpad", "rros-security": "seguridad",
    "rros-devices": "dispositivos", "rros-new": "nuevo", "rros-updates": "actualizar",
}


def main():
    os.makedirs(HICOLOR, exist_ok=True)
    os.makedirs(APPICONS, exist_ok=True)
    for icon, g in APPS.items():
        with open(os.path.join(HICOLOR, icon + ".svg"), "w") as f:
            f.write(squircle(g, GLYPHS[g]))
    with open(os.path.join(HICOLOR, "rros-symbolic.svg"), "w") as f:
        f.write(symbolic_mark())
    for g in GLYPHS:
        with open(os.path.join(APPICONS, ("app-" if g in APPS.values() else "sec-") + g + ".svg"), "w") as f:
            f.write(squircle(g, GLYPHS[g], shadow=g in APPS.values()))


if __name__ == "__main__":
    main()
