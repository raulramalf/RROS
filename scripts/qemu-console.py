#!/usr/bin/env python3
"""Mantiene encendida una VM de RROS (con consola serie de administrador) para depurar sin reiniciar.

  qemu-console.py start            arranca la VM en segundo plano (tarda varios minutos sin KVM)
  qemu-console.py start --disk F   además conecta el disco virtual F (qcow2; se crea de DISK_GB GB si no existe)
  qemu-console.py start --boot-disk  arranca del disco F (sistema instalado) en vez de la ISO; requiere --disk
  (UEFI=1 usa el firmware OVMF; el disco debe instalarse con --uefi y arrancarse con UEFI=1)
  qemu-console.py exec 'orden'     ejecuta una orden como root dentro de la VM y muestra la salida
  qemu-console.py shot ARCHIVO     captura la pantalla gráfica en un PNG
  qemu-console.py stop             apaga la VM

Variables: PROFILE (lite), RAM_MB (2048), OUT (build/vm-<perfil>), DISK_GB (10), UEFI (0)
"""
import os, select, shutil, signal, socket, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROFILE = os.environ.get("PROFILE", "lite")
OUT = os.environ.get("OUT", os.path.join(ROOT, "build", "vm-" + PROFILE))
SERIAL, MONITOR, PIDFILE = (os.path.join(OUT, n) for n in ("serial.sock", "monitor.sock", "qemu.pid"))


def start(args):
    os.makedirs(OUT, exist_ok=True)
    for f in (SERIAL, MONITOR):
        if os.path.exists(f):
            os.remove(f)
    disk = args[args.index("--disk") + 1] if "--disk" in args else None
    boot_disk = "--boot-disk" in args
    if boot_disk and not disk:
        sys.exit("--boot-disk requiere --disk")
    if disk and not os.path.exists(disk):
        subprocess.check_call(["qemu-img", "create", "-q", "-f", "qcow2", disk, "%sG" % os.environ.get("DISK_GB", "10")])
    iso_dir = os.path.join(ROOT, "build", "iso-" + PROFILE)
    cmd = ["qemu-system-x86_64", "-m", os.environ.get("RAM_MB", "2048"), "-smp", "2", "-display", "none", "-vga", "std", "-no-reboot",
           "-serial", "unix:%s,server,nowait" % SERIAL, "-monitor", "unix:%s,server,nowait" % MONITOR]
    if os.environ.get("UEFI") == "1":
        code = "/usr/share/OVMF/OVMF_CODE_4M.fd"
        vars_ = os.path.join(OUT, "OVMF_VARS.fd")
        if not os.path.exists(vars_):
            shutil.copy("/usr/share/OVMF/OVMF_VARS_4M.fd", vars_)
        cmd += ["-drive", "if=pflash,format=raw,readonly=on,file=" + code, "-drive", "if=pflash,format=raw,file=" + vars_]
    if disk:
        cmd += ["-drive", "file=%s,if=virtio,format=qcow2" % disk]
    if boot_disk:
        cmd += ["-boot", "c"]
    else:
        cmd += ["-kernel", os.path.join(iso_dir, "casper", "vmlinuz"), "-initrd", os.path.join(iso_dir, "casper", "initrd"),
                "-append", "boot=casper quiet console=tty0 console=ttyS0 rros.serialshell",
                "-cdrom", os.path.join(ROOT, "build", "rros-%s.iso" % PROFILE)]
    p = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
    open(PIDFILE, "w").write(str(p.pid))
    print("VM arrancada (pid %d). Usa `exec` cuando haya pasado un rato." % p.pid)


def connect():
    s = socket.socket(socket.AF_UNIX)
    s.connect(SERIAL)
    return s


def drain(s, t):
    end = time.time() + t
    data = b""
    while time.time() < end:
        r, _, _ = select.select([s], [], [], 0.3)
        if r:
            d = s.recv(65536)
            if not d:
                break
            data += d
    return data


def run(cmd, timeout=60):
    s = connect()
    drain(s, 0.5)
    marker = "FIN%d" % int(time.time() * 1000)
    s.sendall(("\n%s; echo %s\n" % (cmd, marker)).encode())
    buf, end = b"", time.time() + timeout
    while time.time() < end:
        buf += drain(s, 1)
        txt = buf.decode(errors="replace").replace("\r", "")
        if txt.count(marker) >= 2 or ("\n" + marker) in txt:
            break
    s.close()
    txt = buf.decode(errors="replace").replace("\r", "")
    txt = txt.split("echo " + marker, 1)[-1]
    lines = [l for l in txt.replace(marker, "").split("\n") if l.strip() and "\x1b[?2004" not in l[:8] or l.strip()]
    return "\n".join(l.replace("\x1b[?2004l", "").replace("\x1b[?2004h", "") for l in lines).strip()


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    c = sys.argv[1]
    if c == "start":
        start(sys.argv[2:])
    elif c == "exec":
        print(run(" ".join(sys.argv[2:])))
    elif c == "shot":
        subprocess.check_call([sys.executable, os.path.join(ROOT, "scripts", "qemu-shot.py"), MONITOR, sys.argv[2]])
    elif c == "stop":
        try:
            os.kill(int(open(PIDFILE).read()), signal.SIGTERM)
            print("VM apagada")
        except (OSError, ValueError):
            print("no hay VM en marcha")
    else:
        sys.exit(__doc__)


if __name__ == "__main__":
    main()
