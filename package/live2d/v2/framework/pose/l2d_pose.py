import json

from .l2d_parts_param import L2DPartsParam
from ...core import UtSystem


class L2DPose:

    def __init__(self):
        self.lastTime = 0
        self.lastModel = None
        self.partsGroups = []

    def updateParam(self, model):
        if model != self.lastModel:
            self.initParam(model)

        self.lastModel = model
        cur_time = UtSystem.getUserTimeMSec()
        delta_time_sec = (0 if (self.lastTime == 0) else (cur_time - self.lastTime) / 1000.0)
        self.lastTime = cur_time
        if delta_time_sec < 0:
            delta_time_sec = 0
        for i in range(len(self.partsGroups)):
            self.normalizePartsOpacityGroup(model, self.partsGroups[i], delta_time_sec)
            self.copyOpacityOtherParts(model, self.partsGroups[i])

    def initParam(self, model):
        for i in range(len(self.partsGroups)):
            parts_group = self.partsGroups[i]
            for j in range(len(parts_group)):
                parts_group[j].initIndex(model)
                parts_index = parts_group[j].partsIndex
                param_index = parts_group[j].paramIndex
                if parts_index < 0:
                    continue
                v = (model.getParamFloat(param_index) != 0)
                model.setPartsOpacity(parts_index, (1.0 if v else 0.0))
                model.setParamFloat(param_index, (1.0 if v else 0.0))
                if parts_group[j].link is None:
                    continue
                for k in range(len(parts_group[j].link)):
                    parts_group[j].link[k].initIndex(model)

    @staticmethod
    def normalizePartsOpacityGroup(model, partsGroup, deltaTimeSec):
        visible_parts = -1
        visible_opacity = 1.0
        clear_time_sec = 0.5
        phi = 0.5
        max_back_opacity = 0.15
        for i in range(len(partsGroup)):
            parts_index = partsGroup[i].partsIndex
            param_index = partsGroup[i].paramIndex
            if parts_index < 0:
                continue
            if model.getParamFloat(param_index) != 0:
                if visible_parts >= 0:
                    break

                visible_parts = i
                visible_opacity = model.getPartsOpacity(parts_index)
                visible_opacity += deltaTimeSec / clear_time_sec
                if visible_opacity > 1:
                    visible_opacity = 1

        if visible_parts < 0:
            visible_parts = 0
            visible_opacity = 1

        for i in range(len(partsGroup)):
            parts_index = partsGroup[i].partsIndex
            if parts_index < 0:
                continue
            if visible_parts == i:
                model.setPartsOpacity(parts_index, visible_opacity)
            else:
                opacity = model.getPartsOpacity(parts_index)
                if visible_opacity < phi:
                    a1 = visible_opacity * (phi - 1) / phi + 1
                else:
                    a1 = (1 - visible_opacity) * phi / (1 - phi)

                back_op = (1 - a1) * (1 - visible_opacity)
                if back_op > max_back_opacity:
                    a1 = 1 - max_back_opacity / (1 - visible_opacity)

                opacity = min(opacity, a1)

                model.setPartsOpacity(parts_index, opacity)

    @staticmethod
    def copyOpacityOtherParts(model, partsGroup):
        for i_group in range(len(partsGroup)):
            parts_param = partsGroup[i_group]
            if parts_param.link is None:
                continue
            if parts_param.partsIndex < 0:
                continue
            opacity = model.getPartsOpacity(parts_param.partsIndex)
            for link_parts in parts_param.link:
                if link_parts.partsIndex < 0:
                    continue
                model.setPartsOpacity(link_parts.partsIndex, opacity)

    @staticmethod
    def load(buf):
        ret = L2DPose()
        js = json.loads(buf)
        pose_list_info = js.get("parts_visible")
        pose_num = len(pose_list_info)
        for i_pose in range(pose_num):
            pose_info = pose_list_info[i_pose]
            id_list_info = pose_info.get("group")
            id_num = len(id_list_info)
            parts_group = []
            for i_group in range(id_num):
                parts_info = id_list_info[i_group]
                parts = L2DPartsParam(parts_info["id"])
                parts_group.append(parts)
                if parts_info.get("link") is None:
                    continue
                link_list_info = parts_info.get("link")
                link_num = len(link_list_info)
                parts.link = []
                for i_link in range(link_num):
                    link_parts = L2DPartsParam(link_list_info[i_link])
                    parts.link.append(link_parts)

            ret.partsGroups.append(parts_group)

        return ret
