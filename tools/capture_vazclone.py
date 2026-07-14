"""Launch the built VAZClone standalone, capture its window, save a PNG — ground truth for the
Load/Save menu regression (is the bottom transport bar clipped by the fixed-height editor?)."""
import time, subprocess, sys
try:
    from pywinauto import Application, Desktop
except ImportError as e:
    print("MISSING pywinauto:", e); sys.exit(1)
EXE = r"C:\APC\y\build\plugins\VAZClone\VAZClone_artefacts\Release\Standalone\VAZClone.exe"
OUT = r"C:\Users\ken98\AppData\Local\Temp\claude\C--APC-y\702f4c83-da56-446e-9f5c-d61876c92bfd\scratchpad\vazclone_menu.png"
proc = subprocess.Popen([EXE]); time.sleep(6.0)
try:
    win = None
    for w in Desktop(backend="uia").windows():
        try:
            t = w.window_text()
            if "VAZClone" in t or "VAZ" in t:
                win = w; break
        except Exception: pass
    if win is None:
        # fall back: largest visible top-level window from our pid
        app = Application(backend="uia").connect(process=proc.pid); win = app.top_window()
    r = win.rectangle()
    print(f"window '{win.window_text()}'  rect={r}  size={r.width()}x{r.height()}")
    img = win.capture_as_image(); img.save(OUT)
    print("saved", OUT, img.size)
finally:
    try: proc.terminate()
    except Exception: pass
