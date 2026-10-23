#!/usr/bin/env python3
"""Temas de cursores de Lily, dibujados con Pillow y escritos en formato Xcursor sin xcursorgen.

Varias variantes de color (las tres primeras, con la paleta Lirio):
  Lily-Lirio    flecha marfil con borde vino fino y la punta en rojo lirio.
  Lily-Marfil   flecha vino con borde marfil: se ve bien sobre fondos claros y oscuros.
  Lily-Grafito  grafito neutro con borde blanco.

Cada cursor se dibuja en una rejilla de 32 unidades, a 4x de resolución, y se reduce con LANCZOS
para suavizar los bordes. El contorno sale de dilatar la silueta, así tiene el mismo grosor en todas
las formas, y la sombra es esa silueta difuminada. Los de espera giran (12 fotogramas, 50 ms).

uso: make-cursors.py [--check] [--sheet DIR]
  (escribe overlay/arch/usr/share/icons/Lily-*)
  --check   relee cada archivo y comprueba tamaños, puntos activos y fotogramas
  --sheet   guarda en DIR una hoja de muestra PNG por tema (32 y 64, fondo claro y oscuro)
"""
import math
import os
import shutil
import struct
import sys

from PIL import Image, ImageChops, ImageDraw, ImageFilter

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "overlay/arch/usr/share/icons")

SIZES = (24, 32, 48, 64)
SS = 4            # supermuestreo
FRAMES = 12       # fotogramas de los cursores de espera
DELAY = 50        # ms por fotograma

THEMES = {
    "Lily-Lirio": dict(
        name="Lily Lirio", comment="Cursores marfil con borde vino y punta rojo lirio",
        fill="#f6ecdf", line="#140609", accent="#c8243c", mark="#f6ecdf", shadow=0.42),
    "Lily-Marfil": dict(
        name="Lily Marfil", comment="Cursores vino con borde marfil, muy visibles en cualquier fondo",
        fill="#140609", line="#f6ecdf", accent="#d9304a", mark="#f6ecdf", shadow=0.38),
    "Lily-Grafito": dict(
        name="Lily Grafito", comment="Cursores grafito neutros con borde blanco",
        fill="#34353a", line="#ffffff", accent="#c8243c", mark="#ffffff", shadow=0.40),
    "Lily-Oceano": dict(
        name="Lily Océano", comment="Cursores azul noche con borde blanco y punta turquesa",
        fill="#12324f", line="#ffffff", accent="#22b8cf", mark="#ffffff", shadow=0.40),
    "Lily-Menta": dict(
        name="Lily Menta", comment="Cursores blancos con borde verde bosque y punta menta",
        fill="#f4fbf6", line="#0f3d2a", accent="#2fbf71", mark="#f4fbf6", shadow=0.40),
    "Lily-Lavanda": dict(
        name="Lily Lavanda", comment="Cursores lavanda con borde morado oscuro",
        fill="#e6dcff", line="#2a1650", accent="#8b5cf6", mark="#e6dcff", shadow=0.40),
    "Lily-Ambar": dict(
        name="Lily Ámbar", comment="Cursores negros con borde blanco y punta ámbar",
        fill="#1b1b1f", line="#ffffff", accent="#f59e0b", mark="#ffffff", shadow=0.40),
    "Lily-Sakura": dict(
        name="Lily Sakura", comment="Cursores rosa claro con borde ciruela",
        fill="#ffe1ec", line="#4a1030", accent="#ec4899", mark="#ffe1ec", shadow=0.40),
    "Lily-Nieve": dict(
        name="Lily Nieve", comment="Cursores blancos clásicos con borde negro y punta azul",
        fill="#ffffff", line="#000000", accent="#3b82f6", mark="#ffffff", shadow=0.45),
}


# ---------------------------------------------------------------- dibujo en unidades de la rejilla

class Canvas:
    """Capas (máscaras L) en unidades de 32; cada capa se pinta con un papel: fill, accent, line o mark."""

    def __init__(self, size):
        self.px = size * SS
        self.k = self.px / 32.0
        self.layers = []

    def layer(self, role):
        m = Image.new("L", (self.px, self.px), 0)
        self.layers.append((role, m))
        return m

    def p(self, pts):
        return [(x * self.k, y * self.k) for x, y in pts]

    def poly(self, m, pts, v=255):
        ImageDraw.Draw(m).polygon(self.p(pts), fill=v)

    def ellipse(self, m, cx, cy, rx, ry=None, v=255):
        ry = rx if ry is None else ry
        k = self.k
        ImageDraw.Draw(m).ellipse([(cx - rx) * k, (cy - ry) * k, (cx + rx) * k, (cy + ry) * k], fill=v)

    def rrect(self, m, x0, y0, x1, y1, r, v=255):
        k = self.k
        ImageDraw.Draw(m).rounded_rectangle([x0 * k, y0 * k, x1 * k, y1 * k], radius=r * k, fill=v)

    def line(self, m, pts, w, v=255):
        d = ImageDraw.Draw(m)
        pp = self.p(pts)
        d.line(pp, fill=v, width=max(1, round(w * self.k)), joint="curve")
        r = w * self.k / 2
        for x, y in (pp[0], pp[-1]):
            d.ellipse([x - r, y - r, x + r, y + r], fill=v)

    def arc(self, m, cx, cy, r, w, a0, a1, v=255, caps=True):
        """Arco grueso de a0 a a1 grados (0 = derecha, sentido horario)."""
        n = max(8, int(abs(a1 - a0) / 4))
        pts = [(cx + r * math.cos(math.radians(a0 + (a1 - a0) * i / n)),
                cy + r * math.sin(math.radians(a0 + (a1 - a0) * i / n))) for i in range(n + 1)]
        d = ImageDraw.Draw(m)
        pp = self.p(pts)
        d.line(pp, fill=v, width=max(1, round(w * self.k)), joint="curve")
        if caps:
            rr = w * self.k / 2
            for x, y in (pp[0], pp[-1]):
                d.ellipse([x - rr, y - rr, x + rr, y + rr], fill=v)

    def render(self, theme):
        px = self.px
        union = Image.new("L", (px, px), 0)
        for _, m in self.layers:
            union = ImageChops.lighter(union, m)
        ow = max(1.05, 1.15 * px / SS / 32) * SS          # grosor del contorno, en píxeles de supermuestreo
        ks = int(round(ow)) * 2 + 1
        outer = union.filter(ImageFilter.MaxFilter(ks))
        # sombra: silueta difuminada y desplazada abajo a la derecha
        sh = outer.filter(ImageFilter.GaussianBlur(1.3 * self.k))
        off = Image.new("L", (px, px), 0)
        off.paste(sh, (round(0.5 * self.k), round(1.0 * self.k)))
        img = Image.new("RGBA", (px, px), (0, 0, 0, 0))
        img.putalpha(off.point(lambda a: int(a * theme["shadow"])))
        def put(color, mask):
            solid = Image.new("RGBA", (px, px), color)
            solid.putalpha(mask)
            img.alpha_composite(solid)
        put(theme["line"], outer)
        for role, m in self.layers:
            put(theme[role], m)
        size = px // SS
        return img.resize((size, size), Image.LANCZOS)


# ---------------------------------------------------------------- piezas

def bez(p0, p1, p2, n=10):
    return [((1 - t) ** 2 * p0[0] + 2 * (1 - t) * t * p1[0] + t * t * p2[0],
             (1 - t) ** 2 * p0[1] + 2 * (1 - t) * t * p1[1] + t * t * p2[1])
            for t in (i / n for i in range(n + 1))]


def arrow_pts(s=1.0, ox=0.0, oy=0.0):
    """Flecha con la cola algo curva; la punta en (3, 2)."""
    pts = [(3, 2)]
    pts += bez((3, 2.6), (2.9, 14), (3.1, 23.4), 8)[1:]
    pts += [(3.6, 23.9), (4.3, 23.8)]
    pts += bez((8.4, 19.6), (9.8, 23.5), (11.6, 27.4), 6)
    pts += [(12.3, 27.9), (13.2, 27.8), (15.3, 26.8), (15.8, 26.1), (15.6, 25.3)]
    pts += bez((13.8, 21.4), (12.6, 19.6), (12.3, 18.3), 6)
    pts += [(18.6, 18.2), (19.2, 17.6), (18.8, 16.8)]
    pts += bez((18.8, 16.8), (10, 9), (3.6, 2.2), 8)
    return [(3 + (x - 3) * s + ox, 2 + (y - 2) * s + oy) for x, y in pts]


def arrow(c, s=1.0):
    body = c.layer("fill")
    c.poly(body, arrow_pts(s))
    acc = c.layer("accent")
    c.poly(acc, arrow_pts(s))
    clip = Image.new("L", acc.size, 0)
    c.poly(clip, [(0, 0), (3 + 7.2 * s, 0), (0, 2 + 7.4 * s)])  # sólo la punta
    acc.paste(ImageChops.multiply(acc, clip))
    return body


def badge(c, cx=23, cy=23, r=5.6):
    m = c.layer("accent")
    c.ellipse(m, cx, cy, r)
    return c.layer("mark")


def spinner(c, cx, cy, r, w, frame):
    ring = c.layer("fill")
    c.arc(ring, cx, cy, r, w, 0, 360, caps=False)
    a = c.layer("accent")
    a0 = frame * 360 / FRAMES - 90
    c.arc(a, cx, cy, r, w * 0.92, a0, a0 + 125)


def rot(pts, ang, cx=16, cy=16):
    ca, sa = math.cos(math.radians(ang)), math.sin(math.radians(ang))
    return [(cx + (x - cx) * ca - (y - cy) * sa, cy + (x - cx) * sa + (y - cy) * ca) for x, y in pts]


def double_arrow(L=12.5, head=5.2, hw=5.4, sw=1.15):
    """Flecha doble horizontal centrada en (16, 16), semilargo L."""
    return [(16 - L, 16), (16 - L + head, 16 - hw), (16 - L + head, 16 - sw), (16 + L - head, 16 - sw),
            (16 + L - head, 16 - hw), (16 + L, 16), (16 + L - head, 16 + hw), (16 + L - head, 16 + sw),
            (16 - L + head, 16 + sw), (16 - L + head, 16 + hw)]


def resize(c, ang):
    m = c.layer("fill")
    c.poly(m, rot(double_arrow(L=11.5 if ang % 90 else 12.5), ang))
    a = c.layer("accent")
    c.ellipse(a, 16, 16, 1.9)


def split(c, vertical):
    m = c.layer("fill")
    ang = 90 if vertical else 0
    c.poly(m, rot(double_arrow(L=13, head=5, hw=5, sw=1.1), ang))
    a = c.layer("accent")
    if vertical:
        c.rrect(a, 6.5, 13.3, 25.5, 14.9, 0.5)
        c.rrect(a, 6.5, 17.1, 25.5, 18.7, 0.5)
    else:
        c.rrect(a, 13.3, 6.5, 14.9, 25.5, 0.5)
        c.rrect(a, 17.1, 6.5, 18.7, 25.5, 0.5)
    c.poly(m, rot([(9, 14.6), (23, 14.6), (23, 17.4), (9, 17.4)], ang + 90))


def fourway(c, cx=16, cy=16, s=1.0):
    m = c.layer("fill")
    da = [(cx + (x - 16) * s, cy + (y - 16) * s) for x, y in double_arrow(L=13, head=5, hw=4.8, sw=1.1)]
    c.poly(m, da)
    c.poly(m, rot(da, 90, cx, cy))
    a = c.layer("accent")
    c.ellipse(a, cx, cy, 2.3 * s)


def ibeam(c, ang=0):
    m = c.layer("fill")
    pts = [[(16, 6.5), (16, 25.5)], [(12.2, 4.8), (14.2, 4.8), (16, 6.6), (17.8, 4.8), (19.8, 4.8)],
           [(12.2, 27.2), (14.2, 27.2), (16, 25.4), (17.8, 27.2), (19.8, 27.2)]]
    for seg in pts:
        c.line(m, rot(seg, ang), 1.9)
    a = c.layer("accent")
    c.line(a, rot([(14.6, 16), (17.4, 16)], ang), 1.6)


def hand_pointer(c):
    m = c.layer("fill")
    c.rrect(m, 9.6, 2.0, 14.4, 18, 2.4)                       # índice
    c.rrect(m, 14.0, 9.6, 18.4, 19, 2.2)
    c.rrect(m, 18.0, 10.6, 22.2, 20, 2.1)
    c.rrect(m, 21.8, 12.4, 25.6, 21, 1.9)
    c.rrect(m, 9.6, 14, 25.6, 27.5, 4.5)                      # palma
    c.poly(m, [(10, 17.5), (5.2, 13.2), (3.6, 14.4), (3.9, 16.4), (9.6, 24.5)])  # pulgar
    c.ellipse(m, 4.6, 14.6, 1.6)
    ln = c.layer("line")
    for x, y0, y1 in ((14.2, 12.4, 16.6), (18.2, 13.2, 17.4), (22.0, 14.8, 18.2)):
        c.line(ln, [(x, y0), (x, y1)], 0.55)
    a = c.layer("accent")
    c.rrect(a, 10.8, 25.2, 24.4, 26.4, 0.6)                   # puño rojo


def hand_open(c, closed=False):
    m = c.layer("fill")
    if closed:
        tops = ((8.4, 11.4), (12.6, 10.4), (16.8, 10.6), (21.0, 11.8))
    else:
        tops = ((8.0, 6.4), (12.2, 3.6), (16.6, 4.2), (21.0, 7.6))
    for x, y in tops:
        c.rrect(m, x, y, x + 4.2, 20, 2.1)
    c.rrect(m, 8.0, 14, 25.2, 28, 4.8)
    if closed:
        c.poly(m, [(9, 19), (6.4, 17), (5.4, 18.2), (8.8, 24)])
    else:
        c.poly(m, [(10, 19), (4.4, 13.6), (2.8, 14.8), (3.2, 16.8), (9.4, 25.5)])
        c.ellipse(m, 3.8, 15.0, 1.6)
    ln = c.layer("line")
    for x, _ in tops[1:]:
        c.line(ln, [(x, (13.6 if closed else 12)), (x, 16.5)], 0.55)
    a = c.layer("accent")
    c.rrect(a, 9.4, 25.6, 23.8, 26.8, 0.6)


def no_sign(c, cx=16, cy=16, r=10.5, w=3.4):
    m = c.layer("fill")
    c.arc(m, cx, cy, r, w + 1.4, 0, 360, caps=False)
    a = c.layer("accent")
    c.arc(a, cx, cy, r, w, 0, 360, caps=False)
    d = r * 0.68
    c.line(a, [(cx - d, cy - d), (cx + d, cy + d)], w)


def plus(c, m, cx, cy, s, w):
    c.line(m, [(cx - s, cy), (cx + s, cy)], w)
    c.line(m, [(cx, cy - s), (cx, cy + s)], w)


def question(c, m, cx, cy, s, w):
    c.arc(m, cx, cy - 1.3 * s, 1.35 * s, w, 190, 400)
    ex, ey = cx + 1.35 * s * math.cos(math.radians(40)), cy - 1.3 * s + 1.35 * s * math.sin(math.radians(40))
    c.line(m, [(ex, ey), (cx, cy + 0.6 * s)], w)
    c.ellipse(m, cx, cy + 2.2 * s, w * 0.62)


def link_glyph(c, m, cx, cy, s, w):
    c.arc(m, cx + 0.3 * s, cy + 1.6 * s, 2.6 * s, w, 180, 290, caps=True)
    hx, hy = cx + 0.3 * s + 2.6 * s * math.cos(math.radians(290)), cy + 1.6 * s + 2.6 * s * math.sin(math.radians(290))
    c.poly(m, [(hx + 2.0 * s, hy), (hx - 1.0 * s, hy - 1.7 * s), (hx - 0.6 * s, hy + 1.9 * s)])


def magnifier(c, sign):
    m = c.layer("fill")
    c.line(m, [(18.2, 18.2), (26.5, 26.5)], 4.2)
    c.ellipse(m, 12.5, 12.5, 9.2)
    ln = c.layer("line")
    c.ellipse(ln, 12.5, 12.5, 7.0)
    lens = c.layer("fill")
    c.ellipse(lens, 12.5, 12.5, 5.9)
    a = c.layer("accent")
    c.line(a, [(9.4, 12.5), (15.6, 12.5)], 2.0)
    if sign == "+":
        c.line(a, [(12.5, 9.4), (12.5, 15.6)], 2.0)


# ---------------------------------------------------------------- cursores: nombre -> (dibujo, punto activo en unidades, animado)

def d_default(c, f): arrow(c)
def d_help(c, f): arrow(c, 0.9); question(c, badge(c), 23, 23, 1.5, 1.4)
def d_copy(c, f): arrow(c, 0.9); plus(c, badge(c), 23, 23, 2.6, 1.6)
def d_alias(c, f): arrow(c, 0.9); link_glyph(c, badge(c), 22.6, 22.6, 1.0, 1.4)
def d_dndmove(c, f): arrow(c, 0.9); fourway(c, 23, 23, 0.42)
def d_dndnone(c, f): arrow(c, 0.9); no_sign(c, 23, 23, 4.4, 1.6)
def d_progress(c, f): arrow(c, 0.9); spinner(c, 23.2, 23.2, 4.6, 2.4, f)
def d_wait(c, f): spinner(c, 16, 16, 10, 4.2, f)
def d_text(c, f): ibeam(c)
def d_vtext(c, f): ibeam(c, 90)
def d_pointer(c, f): hand_pointer(c)
def d_grab(c, f): hand_open(c)
def d_grabbing(c, f): hand_open(c, True)
def d_notallowed(c, f): no_sign(c)
def d_move(c, f): fourway(c)
def d_ns(c, f): resize(c, 90)
def d_ew(c, f): resize(c, 0)
def d_nesw(c, f): resize(c, -45)
def d_nwse(c, f): resize(c, 45)
def d_col(c, f): split(c, False)
def d_row(c, f): split(c, True)
def d_zoomin(c, f): magnifier(c, "+")
def d_zoomout(c, f): magnifier(c, "-")


def d_context(c, f):
    arrow(c, 0.85)
    m = c.layer("fill")
    c.rrect(m, 16.5, 14.5, 29, 29, 1.6)
    ln = c.layer("line")
    for y in (18.2, 21.8):
        c.line(ln, [(19, y), (26.5, y)], 1.0)
    a = c.layer("accent")
    c.line(a, [(19, 25.4), (26.5, 25.4)], 1.2)


def d_crosshair(c, f):
    m = c.layer("fill")
    for seg in ([(16, 3), (16, 12.5)], [(16, 19.5), (16, 29)], [(3, 16), (12.5, 16)], [(19.5, 16), (29, 16)]):
        c.line(m, seg, 1.9)
    a = c.layer("accent")
    c.ellipse(a, 16, 16, 1.5)


def d_cell(c, f):
    m = c.layer("fill")
    c.poly(m, [(13, 5), (19, 5), (19, 13), (27, 13), (27, 19), (19, 19), (19, 27), (13, 27), (13, 19), (5, 19), (5, 13), (13, 13)])
    a = c.layer("accent")
    c.rrect(a, 14.6, 14.6, 17.4, 17.4, 0.5)


def d_x(c, f):
    a = c.layer("accent")
    c.line(a, [(7, 7), (25, 25)], 4.6)
    c.line(a, [(25, 7), (7, 25)], 4.6)


def d_pencil(c, f):
    m = c.layer("fill")
    # lápiz a 45°, punta abajo a la izquierda en (3, 29)
    body = [(3.2, 28.8), (5, 22), (21.5, 5.5), (26.5, 10.5), (10, 27)]
    c.poly(m, body)
    a = c.layer("accent")
    c.poly(a, [(21.5, 5.5), (23.6, 3.4), (28.6, 8.4), (26.5, 10.5)])
    ln = c.layer("line")
    c.poly(ln, [(3.2, 28.8), (4.1, 25.3), (6.7, 27.9)])
    c.line(ln, [(5, 22), (10, 27)], 0.6)


CURSORS = {
    # nombre principal: (dibujo, punto activo, animado, alias)
    "default": (d_default, (3, 2), False, ["left_ptr", "arrow", "top_left_arrow", "left-arrow", "right_ptr"]),
    "pointer": (d_pointer, (12, 2.2), False, ["hand2", "hand1", "hand", "pointing_hand",
                                              "e29285e634086352946a0e7090d73106", "9d800788f1b08800ae810202380a0822"]),
    "text": (d_text, (16, 16), False, ["xterm", "ibeam"]),
    "vertical-text": (d_vtext, (16, 16), False, ["048008013003cff3c00c801001200000"]),
    "wait": (d_wait, (16, 16), True, ["watch"]),
    "progress": (d_progress, (3, 2), True, ["left_ptr_watch", "half-busy",
                                            "3ecb610c1bf2410f44200f48c40d3599", "08e8e1c95fe2fc01f976f1e063a24ccd"]),
    "help": (d_help, (3, 2), False, ["question_arrow", "whats_this", "left_ptr_help", "dnd-ask",
                                     "5c6cd98b3f3ebcb1f9c7f1c204630408", "d9ce0ab605698f320427677b458ad60b"]),
    "crosshair": (d_crosshair, (16, 16), False, ["cross", "tcross", "cross_reverse", "diamond_cross"]),
    "cell": (d_cell, (16, 16), False, ["plus"]),
    "move": (d_move, (16, 16), False, ["fleur", "all-scroll", "size_all",
                                       "4498f0e0c1937ffe01fd06f973665830", "9081237383d90e509aa00f00170e968f"]),
    "grab": (d_grab, (16, 14), False, ["openhand", "9141b49c8149039304290b508d208c40", "5aca4d189052212118709018842178c0"]),
    "grabbing": (d_grabbing, (16, 16), False, ["closedhand", "dnd-none-hand", "fcf21c00b30f7e3f83fe0dfd12e71cff"]),
    "not-allowed": (d_notallowed, (16, 16), False, ["forbidden", "crossed_circle", "circle", "no-drop",
                                                    "03b6e0fcb3499374a867c041f52298f0", "03b6e0fcb3499374a867d041f52298f0"]),
    "copy": (d_copy, (3, 2), False, ["dnd-copy", "1081e37283d90000800003c07f3ef6bf",
                                     "6407b0e94181790501fd1e167b474872", "b66166c04f8c3109214a4fbd64a50fc8"]),
    "alias": (d_alias, (3, 2), False, ["link", "dnd-link", "640fb0e74195791501fd1ed57b41487f",
                                       "3085a0e285430894940527032f8b26df", "a2a266d0498c3104214a47bd64ab0fc8"]),
    "dnd-move": (d_dndmove, (3, 2), False, []),
    "dnd-none": (d_dndnone, (3, 2), False, []),
    "context-menu": (d_context, (3, 2), False, []),
    "col-resize": (d_col, (16, 16), False, ["split_h", "14fef782d02440884392942c11205230", "043a9f68147c53184671403ffa811cc8"]),
    "row-resize": (d_row, (16, 16), False, ["split_v", "2870a09082c103050810ffdffffe0204"]),
    "ns-resize": (d_ns, (16, 16), False, ["n-resize", "s-resize", "sb_v_double_arrow", "size_ver", "v_double_arrow",
                                          "top_side", "bottom_side", "00008160000006810000408080010102"]),
    "ew-resize": (d_ew, (16, 16), False, ["e-resize", "w-resize", "sb_h_double_arrow", "size_hor", "h_double_arrow",
                                          "left_side", "right_side", "028006030e0e7ebffc7f7070c0600140"]),
    "nesw-resize": (d_nesw, (16, 16), False, ["ne-resize", "sw-resize", "size_bdiag", "top_right_corner",
                                              "bottom_left_corner", "bd_double_arrow", "fcf1c3c7cd4491d801f1e1c78f100000"]),
    "nwse-resize": (d_nwse, (16, 16), False, ["nw-resize", "se-resize", "size_fdiag", "top_left_corner",
                                              "bottom_right_corner", "fd_double_arrow", "c7088f0f3e6c8088236ef8e1e3e70000"]),
    "zoom-in": (d_zoomin, (12.5, 12.5), False, ["f41c0e382c94c0958e07017e42b00462"]),
    "zoom-out": (d_zoomout, (12.5, 12.5), False, ["f41c0e382c97c0938e07017e42800402"]),
    "pencil": (d_pencil, (3, 29), False, ["draft"]),
    "X_cursor": (d_x, (16, 16), False, ["pirate", "x-cursor"]),
}


# ---------------------------------------------------------------- formato Xcursor

XCUR_IMAGE = 0xFFFD0002


def premul(img):
    out = bytearray()
    for r, g, b, a in img.getdata():
        out += struct.pack("<I", (a << 24) | ((r * a + 127) // 255 << 16) | ((g * a + 127) // 255 << 8) | ((b * a + 127) // 255))
    return bytes(out)


def write_xcursor(path, images):
    """images: lista de (tamaño nominal, xhot, yhot, retardo, imagen RGBA)."""
    n = len(images)
    pos = 16 + 12 * n
    head = struct.pack("<4sIII", b"Xcur", 16, 0x10000, n)
    toc, chunks = b"", b""
    for nom, xh, yh, delay, img in images:
        w, h = img.size
        toc += struct.pack("<III", XCUR_IMAGE, nom, pos + len(chunks))
        chunks += struct.pack("<IIIIIIIII", 36, XCUR_IMAGE, nom, 1, w, h, xh, yh, delay) + premul(img)
    with open(path, "wb") as fh:
        fh.write(head + toc + chunks)


def read_xcursor(path):
    data = open(path, "rb").read()
    magic, hs, ver, n = struct.unpack_from("<4sIII", data, 0)
    assert magic == b"Xcur" and hs == 16 and ver == 0x10000, path
    out = []
    for i in range(n):
        typ, sub, pos = struct.unpack_from("<III", data, 16 + 12 * i)
        assert typ == XCUR_IMAGE
        h36, t, nom, v, w, h, xh, yh, delay = struct.unpack_from("<IIIIIIIII", data, pos)
        assert h36 == 36 and t == XCUR_IMAGE and nom == sub and v == 1
        assert xh < w and yh < h
        px = struct.unpack_from("<%dI" % (w * h), data, pos + 36)
        img = Image.new("RGBA", (w, h))
        img.putdata([((p >> 16 & 255) * 255 // max(1, p >> 24), (p >> 8 & 255) * 255 // max(1, p >> 24),
                      (p & 255) * 255 // max(1, p >> 24), p >> 24) for p in px])
        out.append((nom, xh, yh, delay, img))
    return out


# ---------------------------------------------------------------- temas

def build(theme_dir, theme):
    base = os.path.join(OUT, theme_dir)
    shutil.rmtree(base, ignore_errors=True)
    cdir = os.path.join(base, "cursors")
    os.makedirs(cdir)
    for name, (draw, (hx, hy), animated, aliases) in CURSORS.items():
        images = []
        for size in SIZES:
            for f in range(FRAMES if animated else 1):
                c = Canvas(size)
                draw(c, f)
                img = c.render(theme)
                k = size / 32.0
                images.append((size, min(size - 1, int(hx * k)), min(size - 1, int(hy * k)),
                               DELAY if animated else 0, img))
        write_xcursor(os.path.join(cdir, name), images)
        for al in aliases:
            os.symlink(name, os.path.join(cdir, al))
    meta = "[Icon Theme]\nName=%s\nComment=%s\nInherits=breeze_cursors\n" % (theme["name"], theme["comment"])
    for fn in ("index.theme", "cursor.theme"):
        with open(os.path.join(base, fn), "w") as fh:
            fh.write(meta)


def check(theme_dir):
    cdir = os.path.join(OUT, theme_dir, "cursors")
    for name, (_, (hx, hy), animated, aliases) in CURSORS.items():
        imgs = read_xcursor(os.path.join(cdir, name))
        frames = FRAMES if animated else 1
        assert len(imgs) == len(SIZES) * frames, name
        for size in SIZES:
            sel = [i for i in imgs if i[0] == size]
            assert len(sel) == frames, (name, size)
            for nom, xh, yh, delay, img in sel:
                assert img.size == (size, size)
                assert (xh, yh) == (min(size - 1, int(hx * size / 32)), min(size - 1, int(hy * size / 32))), name
                assert delay == (DELAY if animated else 0)
        for al in aliases:
            p = os.path.join(cdir, al)
            assert os.path.islink(p) and os.readlink(p) == name, al


def sheet(theme_dir, dest):
    cdir = os.path.join(OUT, theme_dir, "cursors")
    names = list(CURSORS)
    cols = 10
    rows = (len(names) + cols - 1) // cols
    cell = 80
    W, H = cols * cell, rows * cell * 4
    out = Image.new("RGBA", (W, H))
    for bi, bg in enumerate(("#faf5ef", "#140609")):
        for si, size in enumerate((32, 64)):
            y0 = (bi * 2 + si) * rows * cell
            out.paste(Image.new("RGBA", (W, rows * cell), bg), (0, y0))
            for i, name in enumerate(names):
                imgs = [im for im in read_xcursor(os.path.join(cdir, name)) if im[0] == size]
                nom, xh, yh, _, img = imgs[len(imgs) // 3]
                x = (i % cols) * cell + (cell - size) // 2
                y = y0 + (i // cols) * cell + (cell - size) // 2
                out.alpha_composite(img, (x, y))
                out.putpixel((x + xh, y + yh), (0, 200, 255, 255))  # punto activo
    out.save(os.path.join(dest, theme_dir + ".png"))


def main():
    args = sys.argv[1:]
    for theme_dir, theme in THEMES.items():
        build(theme_dir, theme)
        if "--check" in args:
            check(theme_dir)
        if "--sheet" in args:
            dest = args[args.index("--sheet") + 1]
            os.makedirs(dest, exist_ok=True)
            sheet(theme_dir, dest)
        print(theme_dir)
    # build() borra cada carpeta: la muestra de Ajustes (preview.png) se vuelve a hacer siempre
    import subprocess
    subprocess.run([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), "cursor-preview.py")]
                   + [os.path.join(OUT, t) for t in THEMES], check=True)


if __name__ == "__main__":
    main()
