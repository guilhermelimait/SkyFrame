"""Grab screenshots from the board over USB (debug channel).
   python tools/shot.py out_dir [commands…]   e.g.  t=now demo s=0 shot s=1 shot
"""
import os, sys, time, serial, serial.tools.list_ports
from PIL import Image

port = next(p.device for p in serial.tools.list_ports.comports() if p.vid == 0x303A)
s = serial.Serial(); s.port, s.baudrate, s.timeout = port, 115200, 0.5
s.dtr = s.rts = False
s.open()
out = sys.argv[1]; os.makedirs(out, exist_ok=True)
n = 0

def cmd(c):
    s.reset_input_buffer()
    s.write((c + "\n").encode())

def shot(name):
    cmd("dump")
    buf = b""; t = time.time()
    while b"FB480\n" not in buf and time.time() - t < 5: buf += s.read(4096)
    if b"FB480\n" not in buf: print("no frame"); return
    data = buf.split(b"FB480\n", 1)[1]
    while len(data) < 480 * 480 * 2 and time.time() - t < 20: data += s.read(65536)
    data = data[:480 * 480 * 2]
    img = Image.new("RGB", (480, 480)); px = img.load()
    for i in range(0, len(data), 2):
        v = data[i] | (data[i + 1] << 8); j = i // 2
        px[j % 480, j // 480] = (((v >> 11) & 31) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31)
    img.save(os.path.join(out, name + ".png")); print("saved", name)

for a in sys.argv[2:]:
    if a == "shot": n += 1; shot(f"{n:02d}")
    elif a.startswith("shot="): shot(a[5:])
    elif a == "t=now": cmd(f"t {int(time.time())}")
    elif a.startswith("t="): cmd("t " + a[2:])
    elif a.startswith("s="): cmd("s " + a[2:])
    elif a.startswith("wait="): time.sleep(float(a[5:]))
    else: cmd(a)
    time.sleep(0.4)
