# max support python version 3.12
import os
import mediapipe as mp
import cv2
import time
import numpy as np
from mediapipe import solutions
from mediapipe.framework.formats import landmark_pb2

from mediapipe_capture.calculation import *
from mediapipe_capture.new_capture_config import *
from mediapipe_capture.math_utils import *

from typing import TYPE_CHECKING
if TYPE_CHECKING:
    from ..facial_params import Params


def draw_landmarks_on_image(rgb_image, detection_result):
  face_landmarks_list = detection_result.face_landmarks
  annotated_image = np.copy(rgb_image)

  # Loop through the detected faces to visualize.
  for idx in range(len(face_landmarks_list)):
    face_landmarks = face_landmarks_list[idx]

    # Draw the face landmarks.
    face_landmarks_proto = landmark_pb2.NormalizedLandmarkList()
    face_landmarks_proto.landmark.extend([
      landmark_pb2.NormalizedLandmark(x=landmark.x, y=landmark.y, z=landmark.z) for landmark in face_landmarks
    ])

    # solutions.drawing_utils.draw_landmarks(
    #     image=annotated_image,
    #     landmark_list=face_landmarks_proto,
    #     connections=mp.solutions.face_mesh.FACEMESH_TESSELATION,
    #     landmark_drawing_spec=None,
    #     connection_drawing_spec=mp.solutions.drawing_styles
    #     .get_default_face_mesh_tesselation_style())
    solutions.drawing_utils.draw_landmarks(
        image=annotated_image,
        landmark_list=face_landmarks_proto,
        connections=mp.solutions.face_mesh.FACEMESH_CONTOURS,
        landmark_drawing_spec=None,
        connection_drawing_spec=mp.solutions.drawing_styles
        .get_default_face_mesh_contours_style())
    solutions.drawing_utils.draw_landmarks(
        image=annotated_image,
        landmark_list=face_landmarks_proto,
        connections=mp.solutions.face_mesh.FACEMESH_IRISES,
          landmark_drawing_spec=None,
          connection_drawing_spec=mp.solutions.drawing_styles
          .get_default_face_mesh_iris_connections_style())

  return annotated_image

cd = os.path.split(__file__)[0]

__face_landmarker_model_path = os.path.join(cd, "face_landmarker.task")

BaseOptions = mp.tasks.BaseOptions
FaceLandmarker = mp.tasks.vision.FaceLandmarker
FaceLandmarkerOptions = mp.tasks.vision.FaceLandmarkerOptions
FaceLandmarkerResult = mp.tasks.vision.FaceLandmarkerResult
VisionRunningMode = mp.tasks.vision.RunningMode

# Create a face landmarker instance with the live stream mode:
def show_frame(result, output_image: mp.Image, timestamp_ms: int):

    if result:
        output_image = draw_landmarks_on_image(output_image.numpy_view(), result)
    else:
        output_image = output_image.numpy_view()

    cv2.imshow("Face Capture", output_image)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        exit()


options = FaceLandmarkerOptions(
    base_options=BaseOptions(model_asset_path=__face_landmarker_model_path),
    running_mode=VisionRunningMode.VIDEO,
    # result_callback=print_result,
    num_faces=1,
)

def capture_task(params: 'Params' = None, source = 0):

    with FaceLandmarker.create_from_options(options) as landmarker:
        cap = cv2.VideoCapture(source)

        while True:
            ret, frame = cap.read()
            if not ret:
                break

            frame = cv2.flip(frame, 1)
            np_frame = np.array(frame)
            mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=np_frame)
            results = landmarker.detect_for_video(mp_image, int(time.time() * 1000))
            show_frame(results, mp_image, int(time.time() * 1000))
        
            face_landmarks_list = results.face_landmarks

            if len(face_landmarks_list) <= 0:
                continue

            landmarks = face_landmarks_list[0]

            # left eye openness ratio
            leftEyePoints = [(landmarks[i].x, landmarks[i].y) for i in LEFT_EYE]

            lEyeOpenRatio = calculate_eye_openness(leftEyePoints)
            # right eye openness ratio
            rightEyePoints = [(landmarks[i].x, landmarks[i].y) for i in RIGHT_EYE]
            rEyeOpenRatio = calculate_eye_openness(rightEyePoints)
            # mouth openness ration
            mouthPoints = [(landmarks[i].x, landmarks[i].y) for i in LIP]
            mouthOpenRatio = calculate_mouth_openness(mouthPoints)

            mouthCorners = [(landmarks[i].x, landmarks[i].y) for i in LIP_CORNER]
            mouthForm = calculate_mouth_form(mouthCorners)

            # 面部角度
            headPoints = [(landmarks[i].x, landmarks[i].y, landmarks[i].z) for i in HEAD]
            roll_angle, yaw_angle, pitch_angle = calculate_head_pose(headPoints)

            # iris
            irisPoints = [(landmarks[i].x, landmarks[i].y) for i in LEFT_EYE_BALL + RIGHT_EYE_BALL]
            eyeBallX = calculate_eye_ball_x(irisPoints)

            if params:
                # 直接使用计算结果会存在抖动现象，需要后续处理
                params.EyeLOpen = round(linearScale01(lEyeOpenRatio, EYE_OPENNESS_MIN, EYE_OPENNESS_MAX), 1)
                params.EyeROpen = round(linearScale01(rEyeOpenRatio, EYE_OPENNESS_MIN, EYE_OPENNESS_MAX), 1)
                params.MouthOpenY = round(linearScale01(mouthOpenRatio, MOUTH_OPENNESS_MIN, MOUTH_OPENNESS_MAX), 1)
                params.MouthForm = linearScale_11(mouthOpenRatio, 0.2, 1.0)
                params.AngleX = clipValue(yaw_angle, -30, 30)
                params.AngleY = clipValue(pitch_angle, -30, 30)
                params.AngleZ = clipValue(roll_angle, -30, 30)
                params.EyeBallX = linearScale_11(eyeBallX, -0.18, 0.18)
                params.MouthForm = linearScale01(mouthForm, 0.08, 0.14)
                

        cap.release()
        cv2.destroyAllWindows()


if __name__ == "__main__":
    capture_task()

