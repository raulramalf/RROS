#!/usr/bin/env python3
"""Captura la pantalla de una VM de QEMU por su monitor y la guarda como PNG (sin dependencias)."""
import socket, struct, sys, time, zlib, os


def ppm_to_png(ppm_path, png_path):
    with open(ppm_path, "rb") as f:
        data = f.read()
    # cabecera P6 <ancho> <alto> <max>
    parts, pos = [], 0
    while len(parts) < 4:
        while data[pos:pos + 1].isspace():
            pos += 1
        end = pos
        while not data[end:end + 1].isspace():
            end += 1
        parts.append(data[pos:end])
        pos = end
    pos += 1
    w, h = int(parts[1]), int(parts[2])
    raw = data[pos:pos + w * h * 3]
    rows = b"".join(b"\x00" + raw[y * w * 3:(y + 1) * w * 3] for y in range(h))

    def chunk(tag, body):
        c = struct.pack(">I", len(body)) + tag + body
        return c + struct.pack(">I", zlib.crc32(tag + body) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) \
        + chunk(b"IDAT", zlib.compress(rows, 6)) + chunk(b"IEND", b"")
    with open(png_path, "wb") as f:
        f.write(png)
    return w, h


def main():
    sock_path, out = sys.argv[1], sys.argv[2]
    ppm = out + ".ppm"
    s = socket.socket(socket.AF_UNIX)
    s.connect(sock_path)
    s.settimeout(5)
    try:
        s.recv(4096)
    except socket.timeout:
        pass
    s.sendall(("screendump %s\n" % ppm).encode())
    for _ in range(50):
        time.sleep(0.2)
        if os.path.exists(ppm) and os.path.getsize(ppm) > 1000:
            time.sleep(0.3)
            break
    w, h = ppm_to_png(ppm, out)
    os.remove(ppm)
    print("captura %dx%d -> %s" % (w, h, out))


if __name__ == "__main__":
    main()
