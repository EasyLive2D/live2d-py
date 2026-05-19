# 简易面捕示例 (mediapipe >=0.10.22)
import os
import threading as td
import time

import cv2
import glfw
import mediapipe as mp
import numpy as np
from mediapipe.tasks.python.vision import FaceLandmarker, FaceLandmarkerOptions, RunningMode
from mediapipe.tasks.python import BaseOptions

import live2d.v2cpp as live2d
# import live2d.v3 as live2d
# import live2d.v2 as live2d
import resources
if live2d.LIVE2D_VERSION == 3:
    from live2d.v3.params import StandardParams
else:
    from live2d.v2.params import StandardParams

live2d.enableLog(False)

# ---- 面部特征点索引 ----
LEFT_EYE   = [362, 385, 387, 263, 373, 380]
RIGHT_EYE  = [33, 160, 158, 133, 153, 144]
LIP        = [78, 82, 312, 308, 317, 87]
LIP_CORNER = [61, 291]
HEAD       = [33, 133, 362, 263, 1, 454, 234, 10, 152]
L_EYE_BALL = [473, 362, 263]
R_EYE_BALL = [468, 33, 133]

# ---- 数学工具 ----
def dist(p1, p2):
    return np.sqrt((p1[0] - p2[0]) ** 2 + (p1[1] - p2[1]) ** 2)

def clamp(v, lo, hi):
    return max(lo, min(hi, v))

def lerp01(v, lo, hi):
    return (clamp(v, lo, hi) - lo) / (hi - lo)

def lerp_11(v, lo, hi):
    mid = (lo + hi) / 2
    return (clamp(v, lo, hi) - mid) / (hi - mid)

def eye_openness(pts):
    return ((dist(pts[1], pts[5]) + dist(pts[2], pts[4])) / 2 / dist(pts[0], pts[3]))

def head_pose(pts):
    le = ((pts[0][0] + pts[1][0]) / 2, (pts[0][1] + pts[1][1]) / 2)
    re = ((pts[2][0] + pts[3][0]) / 2, (pts[2][1] + pts[3][1]) / 2)
    roll  = np.degrees(np.arctan((re[1] - le[1]) / (re[0] - le[0])))
    yaw   = np.degrees(np.arcsin((abs(pts[4][0] - pts[6][0]) - abs(pts[5][0] - pts[4][0]))
                                  / (abs(pts[6][0] - pts[4][0]) + abs(pts[5][0] - pts[4][0]))))
    pitch = np.degrees(np.arctan((pts[8][2] - pts[7][2]) / (pts[7][1] - pts[8][1])))
    return roll, yaw, pitch

def eye_ball_x(pts):
    lc = (pts[2][0] + pts[1][0]) / 2
    lx = (pts[0][0] - lc) / (pts[2][0] - pts[1][0]) * 2
    rc = (pts[5][0] + pts[4][0]) / 2
    rx = (pts[3][0] - rc) / (pts[5][0] - pts[4][0]) * 2
    return (lx + rx) / 2

DRAW_INDICES = LEFT_EYE + RIGHT_EYE + LIP + LIP_CORNER + HEAD + L_EYE_BALL + R_EYE_BALL

# ---- 参数平滑 ----
class Params:
    def __init__(self):
        self.EyeLOpen = self.EyeROpen = 0.0
        self.MouthOpenY = self.MouthForm = 0.0
        self.AngleX = self.AngleY = self.AngleZ = 0.0
        self.EyeBallX = 0.0
        self._prev = [0.0] * 8
        self.k = 0.6

    def _smooth(self, v, i):
        self._prev[i] = self.k * self._prev[i] + (1 - self.k) * v
        return self._prev[i]

    def update(self, src: "Params"):
        self.EyeLOpen   = self._smooth(src.EyeLOpen,   0)
        self.EyeROpen   = self._smooth(src.EyeROpen,   1)
        self.MouthOpenY = self._smooth(src.MouthOpenY, 2)
        self.MouthForm  = self._smooth(src.MouthForm,  3)
        self.AngleX     = self._smooth(src.AngleX,     4)
        self.AngleY     = self._smooth(src.AngleY,     5)
        self.AngleZ     = self._smooth(src.AngleZ,     6)
        self.EyeBallX   = self._smooth(src.EyeBallX,   7)

# ---- 人脸捕捉线程 ----
MODEL_PATH = os.path.join(os.path.dirname(__file__), "mediapipe_capture", "face_landmarker.task")

def capture_task(params: Params, source=0):
    _result_container = []

    def on_result(result, img, ts_ms):
        _result_container.append(result)

    options = FaceLandmarkerOptions(
        base_options=BaseOptions(model_asset_path=MODEL_PATH),
        running_mode=RunningMode.LIVE_STREAM,
        result_callback=on_result,
        num_faces=1,
    )
    with FaceLandmarker.create_from_options(options) as landmarker:
        cap = cv2.VideoCapture(source)
        prev_time = time.time()
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            frame = cv2.flip(frame, 1)
            h, w = frame.shape[:2]

            now = time.time()
            fps = 1.0 / (now - prev_time) if (now - prev_time) > 0 else 0
            prev_time = now

            mp_img = mp.Image(image_format=mp.ImageFormat.SRGB, data=np.array(frame))
            landmarker.detect_async(mp_img, int(now * 1000))

            if _result_container:
                result = _result_container.pop(0)
                _result_container.clear()

                if result.face_landmarks:
                    lm = result.face_landmarks[0]

                    # Draw tracking points
                    for idx in DRAW_INDICES:
                        pt = lm[idx]
                        cv2.circle(frame, (int(pt.x * w), int(pt.y * h)),
                                   2, (0, 255, 0), -1, cv2.LINE_AA)

                    src = Params()
                    lEye = [(lm[i].x, lm[i].y) for i in LEFT_EYE]
                    rEye = [(lm[i].x, lm[i].y) for i in RIGHT_EYE]
                    lipPts = [(lm[i].x, lm[i].y) for i in LIP]
                    lipCrn = [(lm[i].x, lm[i].y) for i in LIP_CORNER]
                    headPts = [(lm[i].x, lm[i].y, lm[i].z) for i in HEAD]
                    iris = [(lm[i].x, lm[i].y) for i in L_EYE_BALL + R_EYE_BALL]

                    roll, yaw, pitch = head_pose(headPts)
                    mOpen = eye_openness(lipPts)
                    mForm = dist(lipCrn[0], lipCrn[1])

                    src.EyeLOpen   = round(lerp01(eye_openness(lEye), 0.25, 0.38), 1)
                    src.EyeROpen   = round(lerp01(eye_openness(rEye), 0.25, 0.38), 1)
                    src.MouthOpenY = round(lerp01(mOpen, 0.1, 0.5), 1)
                    src.MouthForm  = lerp01(mForm, 0.08, 0.14)
                    src.AngleX     = clamp(yaw, -30, 30)
                    src.AngleY     = clamp(pitch, -30, 30)
                    src.AngleZ     = clamp(roll, -30, 30)
                    src.EyeBallX   = lerp_11(eye_ball_x(iris), -0.18, 0.18)

                    params.update(src)

            cv2.putText(frame, f"FPS: {fps:.0f}", (10, 30),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
            cv2.imshow("Face Capture", frame)
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        cap.release()
        cv2.destroyAllWindows()

# ---- 主循环 ----
def main():
    if not glfw.init():
        raise RuntimeError("glfw.init() failed")
    w, h = 450, 700
    window = glfw.create_window(w, h, "facial bind", None, None)
    if not window:
        glfw.terminate()
        return
    glfw.make_context_current(window)
    live2d.init()
    live2d.glInit()

    model = live2d.LAppModel()
    if live2d.LIVE2D_VERSION == 3:
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v3/llny/llny.model3.json"))
    else:
        model.LoadModelJson(os.path.join(resources.RESOURCES_DIRECTORY, "v2/托尔/model0.json"))
    model.Resize(w, h)
    model.SetAutoBreathEnable(False)
    model.SetAutoBlinkEnable(False)

    params = Params()
    td.Thread(None, capture_task, "capture", (params, 0), daemon=True).start()

    def on_click(win, btn, act, mods):
        if btn == glfw.MOUSE_BUTTON_LEFT and act == glfw.PRESS:
            model.SetRandomExpression()
    glfw.set_mouse_button_callback(window, on_click)
    glfw.swap_interval(1)

    fps_frames = 0
    fps_timer = time.time()

    while not glfw.window_should_close(window):
        glfw.poll_events()

        model.SetParameterValue(StandardParams.ParamEyeLOpen,   params.EyeLOpen,   1)
        model.SetParameterValue(StandardParams.ParamEyeROpen,   params.EyeROpen,   1)
        model.SetParameterValue(StandardParams.ParamMouthOpenY, params.MouthOpenY, 1)
        model.SetParameterValue(StandardParams.ParamMouthForm,  params.MouthForm,  1)
        model.SetParameterValue(StandardParams.ParamAngleX,     params.AngleX,     1)
        model.SetParameterValue(StandardParams.ParamAngleY,     params.AngleY,     1)
        model.SetParameterValue(StandardParams.ParamAngleZ,     params.AngleZ,     1)
        model.SetParameterValue(StandardParams.ParamEyeBallX,   params.EyeBallX,   1)
        model.SetParameterValue("Param14", 1, 1)

        live2d.clearBuffer()
        model.Update()
        model.Draw()
        glfw.swap_buffers(window)

        fps_frames += 1
        if time.time() - fps_timer >= 1.0:
            glfw.set_window_title(window, f"facial bind - FPS: {fps_frames}")
            fps_frames = 0
            fps_timer = time.time()
        
        time.sleep(1 / 90)

    live2d.dispose()
    glfw.terminate()

if __name__ == "__main__":
    main()
