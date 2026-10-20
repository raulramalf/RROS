#!/usr/bin/env python3
"""El logotipo de RROS en todos los sitios del sistema, a partir de docs/brand/rros-logo.svg.

- PNG (el logo lleva una foto y filtros que no todos los visores de SVG dibujan): iconos del tema hicolor,
  RROS Ajustes y RROS Shell (app/icons/app-rros.png), pantalla de carga, inicio de sesión (SDDM) y GRUB.
- Una versión de una sola tinta con las letras del logo (RR/OS), para la barra superior y los tamaños pequeños:
  app/icons/app-rros-mono.svg y el icono simbólico rros-symbolic.svg.

Los PNG se dibujan con Chromium sin ventana (el de Playwright o el del sistema). Los archivos generados se
guardan en el repositorio: la ISO no necesita Chromium.
uso: make-logo.py
"""
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
LOGO = os.path.join(ROOT, "docs/brand/rros-logo.svg")
ARCH = os.path.join(ROOT, "overlay/arch/usr/share")
HICOLOR = os.path.join(ARCH, "icons/hicolor")
SIZES = (16, 22, 24, 32, 48, 64, 128, 256, 512)


def chromium():
    for c in ("/opt/pw-browsers/chromium-1194/chrome-linux/chrome", shutil.which("chromium"), shutil.which("chromium-browser"),
              shutil.which("google-chrome")):
        if c and os.path.exists(c):
            return c
    for d in sorted(os.listdir("/opt/pw-browsers")) if os.path.isdir("/opt/pw-browsers") else []:
        c = os.path.join("/opt/pw-browsers", d, "chrome-linux/chrome")
        if os.path.exists(c):
            return c
    sys.exit("make-logo.py: hace falta Chromium para dibujar los PNG")


def render(svg, size, out, tmp):
    from PIL import Image  # solo para recortar y reducir

    big = max(size, 512)
    page = os.path.join(tmp, "p.html")
    with open(page, "w") as f:
        f.write(f'<html><body style="margin:0"><img src="file://{svg}" width="{big}" height="{big}" style="display:block"></body></html>')
    shot = os.path.join(tmp, "s.png")
    subprocess.run([chromium(), "--headless", "--no-sandbox", "--disable-gpu", "--hide-scrollbars",
                    f"--window-size={big},{big + 128}", "--default-background-color=00000000", f"--screenshot={shot}",
                    "file://" + page], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    im = Image.open(shot).convert("RGBA").crop((0, 0, big, big))
    if size != big:
        im = im.resize((size, size), Image.LANCZOS)
    os.makedirs(os.path.dirname(out), exist_ok=True)
    im.save(out, optimize=True)


def mono(color, symbolic=False):
    # las letras del logo (los mismos trazos), sin la foto, recortadas al bloque RR/OS
    paths = re.findall(r'<path transform="[^"]+" d="[^"]+"/>', open(LOGO).read())
    if len(paths) != 4:
        sys.exit("make-logo.py: el logo debería tener 4 letras (RR/OS) en trazos")
    fill = 'class="ColorScheme-Text" fill="currentColor"' if symbolic else f'fill="{color}"'
    style = '<style id="current-color-scheme">.ColorScheme-Text { color:#e6ecff; }</style>' if symbolic else ""
    return (f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="138 140 236 236" width="{16 if symbolic else 64}" '
            f'height="{16 if symbolic else 64}">{style}<g {fill}>' + "".join(paths) + "</g></svg>\n")


def main():
    with tempfile.TemporaryDirectory() as tmp:
        for s in SIZES:
            render(LOGO, s, os.path.join(HICOLOR, f"{s}x{s}/apps/rros.png"), tmp)
        render(LOGO, 512, os.path.join(ROOT, "app/icons/app-rros.png"), tmp)
        render(LOGO, 256, os.path.join(ARCH, "plasma/look-and-feel/org.rros.desktop/contents/splash/images/rros.png"), tmp)
        render(LOGO, 128, os.path.join(ARCH, "sddm/themes/rros/rros.png"), tmp)
        render(LOGO, 96, os.path.join(ARCH, "grub/themes/rros/logo.png"), tmp)
    with open(os.path.join(ROOT, "app/icons/app-rros-mono.svg"), "w") as f:
        f.write(mono("#f4ead9"))
    with open(os.path.join(ROOT, "app/icons/app-rros-mono-dark.svg"), "w") as f:   # para los temas claros
        f.write(mono("#2a0f16"))
    with open(os.path.join(HICOLOR, "scalable/apps/rros-symbolic.svg"), "w") as f:
        f.write(mono(None, symbolic=True))
    # el SVG con la foto no se instala como icono: se usarían los PNG
    for old in (os.path.join(HICOLOR, "scalable/apps/rros.svg"), os.path.join(ROOT, "app/icons/app-rros.svg"),
                os.path.join(ARCH, "plasma/look-and-feel/org.rros.desktop/contents/splash/images/rros.svg"),
                os.path.join(ARCH, "sddm/themes/rros/rros.svg")):
        if os.path.exists(old):
            os.remove(old)


if __name__ == "__main__":
    main()
