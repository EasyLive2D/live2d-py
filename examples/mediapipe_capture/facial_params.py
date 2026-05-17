class Params:
    """
    面部参数
    """
    def __init__(self):
        self.EyeLOpen: float = 0.0
        self.EyeROpen: float = 0.0
        self.MouthOpenY: float = 0.0
        self.MouthForm: float = 0.0
        self.AngleX: float = 0.0
        self.AngleY: float = 0.0
        self.AngleZ: float = 0.0
        self.BodyAngleX: float = 0.0
        self.BodyAngleY: float = 0.0
        self.BodyAngleZ: float = 0.0
        self.EyeBallX: float = 0.0
        # 前一帧参数存储
        self.prev_EyeLOpen = 0
        self.prev_EyeROpen = 0
        self.prev_MouthOpenY = 0
        self.prev_MouthForm = 0
        self.prev_AngleX = 0
        self.prev_AngleY = 0
        self.prev_AngleZ = 0
        self.prev_BodyAngleX = 0
        self.prev_EyeBallX = 0
        # 滤波因子
        self.smooth_factor = 0.5  # 0~1之间，值越大越平滑画面迟滞越明显，值越小抖动越明显画面变化越灵敏

    def smooth(self, current_value, prev_value):
        return self.smooth_factor * prev_value + (1 - self.smooth_factor) * current_value

    def update_params(self, new_params):
        self.EyeLOpen = self.smooth(new_params.EyeLOpen, self.prev_EyeLOpen)
        self.prev_EyeLOpen = self.EyeLOpen

        self.EyeROpen = self.smooth(new_params.EyeROpen, self.prev_EyeROpen)
        self.prev_EyeROpen = self.EyeROpen

        self.MouthOpenY = self.smooth(new_params.MouthOpenY, self.prev_MouthOpenY)
        self.prev_MouthOpenY = self.MouthOpenY

        self.AngleX = self.smooth(new_params.AngleX, self.prev_AngleX)
        self.prev_AngleX = self.AngleX

        self.AngleY = self.smooth(new_params.AngleY, self.prev_AngleY)
        self.prev_AngleY = self.AngleY

        self.AngleZ = self.smooth(new_params.AngleZ, self.prev_AngleZ)
        self.prev_AngleZ = self.AngleZ

        self.BodyAngleX = self.smooth(new_params.BodyAngleX, self.prev_BodyAngleX)
        self.prev_BodyAngleX = self.BodyAngleX

        self.EyeBallX = self.smooth(new_params.EyeBallX, self.prev_EyeBallX)
        self.prev_EyeBallX = self.EyeBallX

        self.MouthForm = self.smooth(new_params.MouthForm, self.prev_MouthForm)
        self.prev_MouthForm = self.MouthForm