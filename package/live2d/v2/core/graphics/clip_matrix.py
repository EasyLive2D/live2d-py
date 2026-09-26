class ClipMatrix:

    def __init__(self):
        self.m = [0.0] * (16)
        self.identity()

    def identity(self):
        for aH in range(0, 16, 1):
            self.m[aH] = 1 if ((aH % 5) == 0) else 0

    def getArray(self):
        return self.m

    def getCopyMatrix(self):
        return [0.0] * len(self.m)

    def setMatrix(self, aI):
        if aI is None or len(aI) != 16:
            return

        for aH in range(0, 16, 1):
            self.m[aH] = aI[aH]

    def translate(self, aH, aJ, aI):
        self.m[12] = self.m[0] * aH + self.m[4] * aJ + self.m[8] * aI + self.m[12]
        self.m[13] = self.m[1] * aH + self.m[5] * aJ + self.m[9] * aI + self.m[13]
        self.m[14] = self.m[2] * aH + self.m[6] * aJ + self.m[10] * aI + self.m[14]
        self.m[15] = self.m[3] * aH + self.m[7] * aJ + self.m[11] * aI + self.m[15]

    def scale(self, aJ, aI, aH):
        self.m[0] *= aJ
        self.m[4] *= aI
        self.m[8] *= aH
        self.m[1] *= aJ
        self.m[5] *= aI
        self.m[9] *= aH
        self.m[2] *= aJ
        self.m[6] *= aI
        self.m[10] *= aH
        self.m[3] *= aJ
        self.m[7] *= aI
        self.m[11] *= aH
