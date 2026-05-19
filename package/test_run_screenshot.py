"""Headless screenshot test for CI workflow validation.
Usage:
  python test_run_screenshot.py v2cpp                    # default model + expr
  python test_run_screenshot.py v2cpp --model shizuku    # specific model
  python test_run_screenshot.py v2cpp --expr f05.exp.json  # specific expression
  python test_run_screenshot.py v3   --model Haru
  python test_run_screenshot.py v2   --model shizuku
"""
import argparse
import os
import sys
import time

import OpenGL.GL as gl
import numpy as np
from PIL import Image
from PySide6.QtCore import QTimerEvent, Qt
from PySide6.QtGui import QGuiApplication
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from PySide6.QtWidgets import QApplication
from PySide6.QtCore import QTimer

parser = argparse.ArgumentParser(description="Live2D screenshot test")
parser.add_argument("version", choices=["v2", "v2cpp", "v3"], help="SDK version")
parser.add_argument("--model", default="", help="Model name (subdir under Resources/v{version}/)")
parser.add_argument("--expr", default="", help="Expression file name to set")
parser.add_argument("--frames", type=int, default=1, help="Number of frames to render before screenshot")
parser.add_argument("--output", default="", help="Output PNG path (default: screenshot_<version>.png)")
parser.add_argument("--width", type=int, default=400, help="Canvas width")
parser.add_argument("--height", type=int, default=500, help="Canvas height")
args = parser.parse_args()

# ---- Import the right SDK ----
if args.version == "v2cpp":
    import live2d.v2cpp as live2d
elif args.version == "v2":
    import live2d.v2 as live2d
else:
    import live2d.v3 as live2d

# ---- Resolve model & resources ----
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
RESOURCES = os.path.join(SCRIPT_DIR, "..", "Resources")

MODEL_CONFIGS = {
    "v2": {
        "":        ("v2/托尔/model0.json",              "f01.exp.json"),
        "tor":     ("v2/托尔/model0.json",              "f01.exp.json"),
        "haru":    ("v2/haru/haru.model.json",           "f01.exp.json"),
        "kasumi2": ("v2/kasumi2/kasumi2.model.json",     None),
        "shizuku": ("v2/shizuku/shizuku.model.json",     None),
    },
    "v2cpp": {
        "":        ("v2/托尔/model0.json",              "f01.exp.json"),
        "tor":     ("v2/托尔/model0.json",              "f01.exp.json"),
        "haru":    ("v2/haru/haru.model.json",           "f01.exp.json"),
        "kasumi2": ("v2/kasumi2/kasumi2.model.json",     None),
        "shizuku": ("v2/shizuku/shizuku.model.json",     None),
    },
    "v3": {
        "":        ("v3/Haru/Haru.model3.json",          None),
        "haru":    ("v3/Haru/Haru.model3.json",          None),
        "mao":     ("v3/Mao/Mao.model3.json",            None),
        "llny":    ("v3/llny/llny.model3.json",          None),
    },
}

model_key = args.model.lower()
cfg = MODEL_CONFIGS.get(args.version, {}).get(model_key)
if cfg is None:
    print(f"[test] unknown model '{args.model}' for {args.version}", file=sys.stderr)
    sys.exit(1)
model_json = os.path.join(RESOURCES, cfg[0])
expr_name = args.expr or cfg[1]

output_png = args.output or f"screenshot_{args.version}.png"

print(f"[test] version={args.version} model={model_json}")
print(f"[test] expression={expr_name} output={output_png}")

# ---- OpenGL widget ----
class Win(QOpenGLWidget):
    def __init__(self):
        super().__init__()
        self.setAttribute(Qt.WidgetAttribute.WA_TranslucentBackground, True)
        self.resize(args.width, args.height)
        self.model = None
        self.done = False
        self.frame_count = 0
        self.systemScale = QGuiApplication.primaryScreen().devicePixelRatio()

    def initializeGL(self):
        live2d.glInit()
        self.model = live2d.LAppModel()
        self.model.LoadModelJson(model_json)
        self.model.Resize(args.width, args.height)
        self.model.SetAutoBlinkEnable(False)
        self.model.SetAutoBreathEnable(False)

        if expr_name:
            self.model.SetExpression(expr_name)

    def resizeGL(self, w, h):
        if self.model:
            self.model.Resize(w, h)

    def paintGL(self):
        if self.done:
            return
        live2d.clearBuffer()
        self.model.Update()
        self.model.Draw()
        self.frame_count += 1

        if self.frame_count >= args.frames:
            self.savePng(output_png)
            self.done = True
            print(f"[test] screenshot saved: {output_png}")
            QTimer.singleShot(100, self._close)

    def savePng(self, fName):
        scale = self.systemScale
        w, h = int(self.width() * scale), int(self.height() * scale)
        data = gl.glReadPixels(0, 0, w, h, gl.GL_RGBA, gl.GL_UNSIGNED_BYTE)
        data = np.frombuffer(data, dtype=np.uint8).reshape(h, w, 4)
        data = np.flipud(data)
        img = Image.fromarray(data, "RGBA")
        img.save(fName)

    def _close(self):
        self.window().close()

    def timerEvent(self, a0: QTimerEvent | None) -> None:
        if self.isVisible():
            self.update()


# ---- Run ----
live2d.init()
app = QApplication(sys.argv)
win = Win()
win.show()

timer = QTimer()
timer.setSingleShot(True)
timer.setInterval(10000)
timer.timeout.connect(lambda: (print("[test] timeout"), win.close()))
timer.start()

rc = app.exec()
live2d.dispose()

if not win.done:
    print("[test] FAILED: no screenshot produced", file=sys.stderr)
    sys.exit(1)

print("[test] OK")
