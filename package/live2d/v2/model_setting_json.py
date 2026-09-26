from typing import Union
import json
import math


class ModelSettingJson:
    NAME = "name"
    ID = "id"
    MODEL = "model"
    TEXTURES = "textures"
    HIT_AREAS = "hit_areas"
    PHYSICS = "physics"
    POSE = "pose"
    EXPRESSIONS = "expressions"
    MOTION_GROUPS = "motions"
    SOUND = "sound"
    FADE_IN = "fade_in"
    FADE_OUT = "fade_out"
    LAYOUT = "layout"
    INIT_PARAM = "init_param"
    INIT_PARTS_VISIBLE = "init_parts_visible"
    VALUE = "val"
    FILE = "file"

    def __init__(self):
        self.json = {}

    def loadModelSetting(self, path) -> None:
        with open(path, 'rb') as f:
            self.json = json.loads(f.read())

    def getTextureFile(self, n) -> Union[str, None]:
        if self.json.get(self.TEXTURES) is None or self.json[self.TEXTURES][n] is None:
            return None
        return self.json[self.TEXTURES][n]

    def getModelFile(self):
        return self.json[self.MODEL]

    def getTextureNum(self) -> int:
        if self.json.get(self.TEXTURES) is None:
            return 0
        return len(self.json[self.TEXTURES])

    def getHitAreaNum(self):
        if self.json.get(self.HIT_AREAS, None) is None:
            return 0
        return len(self.json[self.HIT_AREAS])

    def getHitAreaID(self, n):
        if (
            self.json.get(self.HIT_AREAS, None) is None
            or len(self.json[self.HIT_AREAS]) == 0
        ):
            return None
        return self.json[self.HIT_AREAS][n][self.ID]

    def getHitAreaName(self, n):
        if (
            self.json.get(self.HIT_AREAS, None) is None
            or len(self.json[self.HIT_AREAS]) == 0
        ):
            return None
        return self.json[self.HIT_AREAS][n][self.NAME]

    def getPhysicsFile(self):
        return self.json.get(self.PHYSICS)

    def getPoseFile(self):
        return self.json.get(self.POSE)

    def getExpressionNum(self):
        return (
            0
            if (self.json.get(self.EXPRESSIONS, None) is None)
            else len(self.json[self.EXPRESSIONS])
        )

    def getExpressionFile(self, n):
        if self.json.get(self.EXPRESSIONS, None) is None:
            return None
        return self.json[self.EXPRESSIONS][n][self.FILE]

    def getExpressionName(self, n):
        if self.json.get(self.EXPRESSIONS, None) is None:
            return None
        return self.json[self.EXPRESSIONS][n][self.NAME]

    def getLayout(self):
        return self.json.get(self.LAYOUT)

    def getInitParamNum(self):
        return (
            0
            if (self.json.get(self.INIT_PARAM, None) is None)
            else len(self.json[self.INIT_PARAM])
        )

    def getMotionNum(self, name):
        if (
            self.json.get(self.MOTION_GROUPS, None) is None
            or self.json[self.MOTION_GROUPS].get(name, None) is None
        ):
            return 0
        return len(self.json[self.MOTION_GROUPS][name])

    def getMotionFile(self, name, n):
        if (
            self.json.get(self.MOTION_GROUPS, None) is None
            or self.json[self.MOTION_GROUPS].get(name, None) is None
            or self.json[self.MOTION_GROUPS][name][n] is None
        ):
            return None
        return self.json[self.MOTION_GROUPS][name][n][self.FILE]

    def getMotionSound(self, name, n):
        if (
            self.json.get(self.MOTION_GROUPS, None) is None
            or self.json[self.MOTION_GROUPS].get(name, None) is None
            or self.json[self.MOTION_GROUPS][name][n] is None
            or self.json[self.MOTION_GROUPS][name][n].get(self.SOUND, None) is None
        ):
            return None
        return self.json[self.MOTION_GROUPS][name][n][self.SOUND]

    def getMotionFadeIn(self, name, n):
        if (
            self.json.get(self.MOTION_GROUPS, None) is None
            or self.json[self.MOTION_GROUPS].get(name, None) is None
            or self.json[self.MOTION_GROUPS][name][n] is None
            or self.json[self.MOTION_GROUPS][name][n].get(self.FADE_IN, None) is None
        ):
            return 1000
        return self.json[self.MOTION_GROUPS][name][n][self.FADE_IN]

    def getMotionFadeOut(self, name, n):
        if (
            self.json.get(self.MOTION_GROUPS, None) is None
            or self.json[self.MOTION_GROUPS].get(name, None) is None
            or self.json[self.MOTION_GROUPS][name][n] is None
            or self.json[self.MOTION_GROUPS][name][n].get(self.FADE_OUT, None) is None
        ):
            return 1000
        return self.json[self.MOTION_GROUPS][name][n][self.FADE_OUT]

    def getMotionNames(self):
        if self.json.get(self.MOTION_GROUPS, None) is None:
            return None

        return tuple(self.json[self.MOTION_GROUPS].keys())

    def getInitParamID(self, n):
        if (
            self.json.get(self.INIT_PARAM, None) is None
            or self.json[self.INIT_PARAM].get(n, None) is None
        ):
            return None
        return self.json[self.INIT_PARAM][n][self.ID]

    def getInitParamValue(self, n):
        if (
            self.json.get(self.INIT_PARAM, None) is None
            or self.json[self.INIT_PARAM].get(n, None) is None
        ):
            return math.nan
        return self.json[self.INIT_PARAM][n][self.VALUE]

    def getInitPartsVisibleNum(self):
        return (
            0
            if (self.json.get(self.INIT_PARTS_VISIBLE, None) is None)
            else len(self.json[self.INIT_PARTS_VISIBLE])
        )

    def getInitPartsVisibleID(self, n):
        if (
            self.json.get(self.INIT_PARTS_VISIBLE, None) is None
            or self.json[self.INIT_PARTS_VISIBLE].get(n, None) is None
        ):
            return None
        return self.json[self.INIT_PARTS_VISIBLE][n][self.ID]

    def getInitPartsVisibleValue(self, n):
        if (
            self.json.get(self.INIT_PARTS_VISIBLE, None) is None
            or self.json[self.INIT_PARTS_VISIBLE].get(n, None) is None
        ):
            return math.nan
        return self.json[self.INIT_PARTS_VISIBLE][n][self.VALUE]
