__version__ = "0.8.1"
__csm_version__ = "5-r.5"
__official_site__ = "https://www.live2d.com/en/sdk/about/"


def __print_banner():
      import sys
      _enc = (getattr(sys.stdout, "encoding", "") or "").lower()
      _utf8 = "utf" in _enc
      _L = "─" * 48 if _utf8 else "=" * 48
      _S = "★" if _utf8 else "*"
      _D = "·" if _utf8 else "-"

      print(f"\033[95m{_L}\033[0m\n"
            f"  \033[93m{_S}\033[0m \033[1m\033[96mlive2d-py\033[0m  \033[2m{_D}\033[0m  \033[1mNon-official live2d library\033[0m\n"
            f"      \033[2mversion\033[0m  \033[92m\033[1m{__version__}\033[0m\n"
            f"      \033[2m CsmSDK\033[0m  \033[92m\033[1m{__csm_version__}\033[0m\n"
            f"     \033[2mOfficial\033[0m  \033[92m\033[1m{__official_site__}\033[0m\n"
            f"\033[95m{_L}\033[0m")

__print_banner()


from ._live2d import *


class LogLevels:
    LV_DEBUG: int = 0
    LV_INFO: int = 1
    LV_WARN: int = 2
    LV_ERROR: int = 3


class MotionPriority:
    NONE = 0
    IDLE = 1
    NORMAL = 2
    FORCE = 3


def init():
    import os
    __cd = os.path.split(__file__)[0]
    init_internal(str(__cd))


class StandardParamsV2:
    ParamAngleX = "PARAM_ANGLE_X"
    ParamAngleY = "PARAM_ANGLE_Y"
    ParamAngleZ = "PARAM_ANGLE_Z"
    ParamEyeLOpen = "PARAM_EYE_L_OPEN"
    ParamEyeROpen = "PARAM_EYE_R_OPEN"
    ParamEyeLSmile = "PARAM_EYE_L_SMILE"
    ParamEyeRSmile = "PARAM_EYE_R_SMILE"
    ParamEyeBallX = "PARAM_EYE_BALL_X"
    ParamEyeBallY = "PARAM_EYE_BALL_Y"
    ParamEyeBallForm = "PARAM_EYE_BALL_FORM"
    ParamBrowLX = "PARAM_BROW_L_X"
    ParamBrowLY = "PARAM_BROW_L_Y"
    ParamBrowLAngle = "PARAM_BROW_L_ANGLE"
    ParamBrowLForm = "PARAM_BROW_L_FORM"
    ParamBrowRX = "PARAM_BROW_R_X"
    ParamBrowRY = "PARAM_BROW_R_Y"
    ParamBrowRAngle = "PARAM_BROW_R_ANGLE"
    ParamBrowRForm = "PARAM_BROW_R_FORM"
    ParamMouthOpenY = "PARAM_MOUTH_OPEN_Y"
    ParamMouthForm = "PARAM_MOUTH_FORM"
    ParamSmile = "PARAM_SMILE"
    ParamTere = "PARAM_TERE"
    ParamBodyAngleX = "PARAM_BODY_ANGLE_X"
    ParamBodyAngleZ = "PARAM_BODY_ANGLE_Z"
    ParamBreath = "PARAM_BREATH"
    ParamHairFront = "PARAM_HAIR_FRONT"
    ParamHairSide = "PARAM_HAIR_SIDE"
    ParamHairBack = "PARAM_HAIR_BACK"
    ParamHairFuwa = "PARAM_HAIR_FUWA"
    ParamShoulderX = "PARAM_SHOULDER_X"
    ParamBustX = "PARAM_BUST_X"
    ParamBustY = "PARAM_BUST_Y"
    ParamBaseX = "PARAM_BASE_X"
    ParamBaseY = "PARAM_BASE_Y"


class StandardParamsV3:
    ParamAngleX = "ParamAngleX"
    ParamAngleY = "ParamAngleY"
    ParamAngleZ = "ParamAngleZ"
    ParamEyeLOpen = "ParamEyeLOpen"
    ParamEyeLSmile = "ParamEyeLSmile"
    ParamEyeROpen = "ParamEyeROpen"
    ParamEyeRSmile = "ParamEyeRSmile"
    ParamEyeBallX = "ParamEyeBallX"
    ParamEyeBallY = "ParamEyeBallY"
    ParamEyeBallForm = "ParamEyeBallForm"
    ParamBrowLY = "ParamBrowLY"
    ParamBrowRY = "ParamBrowRY"
    ParamBrowLX = "ParamBrowLX"
    ParamBrowRX = "ParamBrowRX"
    ParamBrowLAngle = "ParamBrowLAngle"
    ParamBrowRAngle = "ParamBrowRAngle"
    ParamBrowLForm = "ParamBrowLForm"
    ParamBrowRForm = "ParamBrowRForm"
    ParamMouthForm = "ParamMouthForm"
    ParamMouthOpenY = "ParamMouthOpenY"
    ParamCheek = "ParamCheek"
    ParamBodyAngleX = "ParamBodyAngleX"
    ParamBodyAngleY = "ParamBodyAngleY"
    ParamBodyAngleZ = "ParamBodyAngleZ"
    ParamBreath = "ParamBreath"
    ParamArmLA = "ParamArmLA"
    ParamArmRA = "ParamArmRA"
    ParamArmLB = "ParamArmLB"
    ParamArmRB = "ParamArmRB"
    ParamHandL = "ParamHandL"
    ParamHandR = "ParamHandR"
    ParamHairFront = "ParamHairFront"
    ParamHairSide = "ParamHairSide"
    ParamHairBack = "ParamHairBack"
    ParamHairFluffy = "ParamHairFluffy"
    ParamShoulderY = "ParamShoulderY"
    ParamBustX = "ParamBustX"
    ParamBustY = "ParamBustY"
    ParamBaseX = "ParamBaseX"
    ParamBaseY = "ParamBaseY"