import json

from ...core import PhysicsHair, UtSystem


class L2DPhysics:

    def __init__(self):
        self.physicsList = []
        self.startTimeMSec = UtSystem.getUserTimeMSec()

    def updateParam(self, model):
        time_m_sec = UtSystem.getUserTimeMSec() - self.startTimeMSec
        for i in range(len(self.physicsList)):
            self.physicsList[i].update(model, time_m_sec)

    @staticmethod
    def load(buf):
        ret = L2DPhysics()
        js = json.loads(buf)
        params = js.get("physics_hair")
        param_num = len(params)
        for i in range(param_num):
            param = params[i]
            physics = PhysicsHair()
            setup = param.get("setup")
            length = float(len(setup))
            resist = float(setup.get("regist"))
            mass = float(setup.get("mass"))
            physics.setup(length, resist, mass)
            src_list = param.get("src")
            src_num = len(src_list)
            for j in range(src_num):
                src = src_list[j]
                tid = src.get("id")
                type_str = src.get("ptype")
                if type_str == "x":
                    t = PhysicsHair.SRC_TO_X
                elif type_str == "y":
                    t = PhysicsHair.SRC_TO_Y
                elif type_str == "angle":
                    t = PhysicsHair.SRC_TO_G_ANGLE
                else:
                    raise Exception("error")

                scale = float(src.get("scale"))
                weight = float(src.get("weight"))
                physics.addSrcParam(t, tid, scale, weight)

            target_list = param.get("targets")
            target_num = len(target_list)
            for j in range(target_num):
                target = target_list[j]
                tid = target.get("id")
                type_str = target.get("ptype")
                if type_str == "angle":
                    t = PhysicsHair.TARGET_FROM_ANGLE
                elif type_str == "angle_v":
                    t = PhysicsHair.TARGET_FROM_ANGLE_V
                else:
                    raise Exception("live2d", "Invalid parameter:PhysicsHair.Target")

                scale = float(target.get("scale"))
                weight = float(target.get("weight"))
                physics.addTargetParam(t, tid, scale, weight)

            ret.physicsList.append(physics)

        return ret
