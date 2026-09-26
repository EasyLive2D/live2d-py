import json

from ...core import AMotion
from .l2d_expression_param import L2DExpressionParam


class L2DExpressionMotion(AMotion):
    EXPRESSION_DEFAULT = "DEFAULT"
    TYPE_SET = 0
    TYPE_ADD = 1
    TYPE_MULT = 2

    def __init__(self):
        super().__init__()
        self.paramList = []

    def updateParamExe(self, model, timeMSec, weight, motionQueueEnt):
        for i in range(len(self.paramList) - 1, -1, -1):
            param = self.paramList[i]
            if param.type == L2DExpressionMotion.TYPE_ADD:
                model.addToParamFloat(param.id, param.value, weight)
            elif param.type == L2DExpressionMotion.TYPE_MULT:
                model.multParamFloat(param.id, param.value, weight)
            elif param.type == L2DExpressionMotion.TYPE_SET:
                model.setParamFloat(param.id, param.value, weight)

    @staticmethod
    def loadJson(buf):
        ret = L2DExpressionMotion()
        js = json.loads(buf)
        ret.setFadeIn(int(js.get("fade_in", 0)) if int(js.get("fade_in", 0)) > 0 else 1000)
        ret.setFadeOut(int(js.get("fade_out", 0)) if int(js.get("fade_out", 0)) > 0 else 1000)
        if js.get("params", None) is None:
            return ret

        params = js["params"]
        param_num = len(params)
        ret.paramList = []
        for i in range(0, param_num, 1):
            param = params[i]
            param_id = str(param["id"])
            value = float(param["val"])
            calc = str(param.get("calc", "add"))
            if calc == "add":
                calc_type_int = L2DExpressionMotion.TYPE_ADD
            elif calc == "mult":
                calc_type_int = L2DExpressionMotion.TYPE_MULT
            elif calc == "set":
                calc_type_int = L2DExpressionMotion.TYPE_SET
            else:
                calc_type_int = L2DExpressionMotion.TYPE_ADD

            if calc_type_int == L2DExpressionMotion.TYPE_ADD:
                default_value = float(param.get("def", 0))
                value = value - default_value
            elif calc_type_int == L2DExpressionMotion.TYPE_MULT:
                default_value = float(param.get("def", 1))
                if default_value == 0:
                    default_value = 1
                value = value / default_value

            item = L2DExpressionParam()
            item.id = param_id
            item.type = calc_type_int
            item.value = value
            ret.paramList.append(item)

        return ret
