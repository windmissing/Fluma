import subprocess
import sys
import time
import ctypes
import ctypes.wintypes as wt

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32

class RECT(ctypes.Structure):
    _fields_ = [("left", ctypes.c_long), ("top", ctypes.c_long),
                ("right", ctypes.c_long), ("bottom", ctypes.c_long)]

def find_window(title_part):
    hwnds = []
    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h, l):
        n = ctypes.create_unicode_buffer(256)
        user32.GetWindowTextW(h, n, 256)
        if title_part in n.value:
            hwnds.append(h)
        return True
    user32.EnumWindows(cb, 0)
    return hwnds[0] if hwnds else None

def capture_printwindow(hwnd, out_png):
    # PrintWindow with PW_RENDERFULLCONTENT=2 renders even occluded windows
    rect = RECT()
    user32.GetWindowRect(hwnd, ctypes.byref(rect))
    w = rect.right - rect.left
    h = rect.bottom - rect.top
    hdc = user32.GetDC(hwnd)
    memdc = gdi32.CreateCompatibleDC(hdc)
    bmp = gdi32.CreateCompatibleBitmap(hdc, w, h)
    gdi32.SelectObject(memdc, bmp)
    ok = user32.PrintWindow(hwnd, memdc, 2)
    if not ok:
        # fallback 0
        ok = user32.PrintWindow(hwnd, memdc, 0)
    user32.ReleaseDC(hwnd, hdc)
    class BMI(ctypes.Structure):
        _fields_ = [("biSize", ctypes.c_uint), ("biWidth", ctypes.c_int),
                    ("biHeight", ctypes.c_int), ("biPlanes", ctypes.c_short),
                    ("biBitCount", ctypes.c_short), ("biCompression", ctypes.c_uint),
                    ("biSizeImage", ctypes.c_uint), ("biXPelsPerMeter", ctypes.c_int),
                    ("biYPelsPerMeter", ctypes.c_int), ("biClrUsed", ctypes.c_uint),
                    ("biClrImportant", ctypes.c_uint)]
    bmi = BMI()
    bmi.biSize = ctypes.sizeof(BMI)
    bmi.biWidth = w
    bmi.biHeight = -h
    bmi.biPlanes = 1
    bmi.biBitCount = 32
    bmi.biCompression = 0
    buf = ctypes.create_string_buffer(w * h * 4)
    gdi32.GetDIBits(memdc, bmp, 0, h, buf, ctypes.byref(bmi), 0)
    gdi32.DeleteObject(bmp)
    gdi32.DeleteDC(memdc)
    from PIL import Image
    img = Image.frombytes("RGBA", (w, h), buf.raw, "raw", "BGRA", w*4, 1)
    img.convert("RGB").save(out_png, "PNG")
    return out_png

if __name__ == "__main__":
    exe = sys.argv[1]
    args = sys.argv[2:-2]
    outdir = sys.argv[-2]
    n_shots = int(sys.argv[-1])
    proc = subprocess.Popen([exe] + args + [outdir, "--present"])
    hwnd = None
    t0 = time.time()
    while hwnd is None and time.time() - t0 < 10:
        hwnd = find_window("G14 16x16 Analytic")
        if hwnd is None:
            time.sleep(0.05)
    if hwnd is None:
        print("WINDOW NOT FOUND")
        proc.kill()
        sys.exit(1)
    shots = []
    interval = 3.0 / (n_shots + 1)
    for i in range(n_shots):
        time.sleep(interval)
        out = f"{outdir}/frame-{i+1:02d}.png"
        capture_printwindow(hwnd, out)
        shots.append(out)
    time.sleep(0.8)
    end = f"{outdir}/frame-end.png"
    capture_printwindow(hwnd, end)
    shots.append(end)
    proc.kill()
    proc.wait()
    print("SHOTS " + " ".join(shots))
