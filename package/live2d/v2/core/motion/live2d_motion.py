from ..util import UtString
from .amotion import AMotion
from .motion import Motion


class Live2DMotion(AMotion):
    MTN_PREFIX_VISIBLE = "VISIBLE:"
    MTN_PREFIX_LAYOUT = "LAYOUT:"
    MTN_PREFIX_FADE_IN = "FADEIN:"
    MTN_PREFIX_FADE_OUT = "FADEOUT:"

    def __init__(self):

        super().__init__()
        self.motions = []
        self.srcFps = 30
        self.maxLength = 0
        self.loop = False
        self.loopFadeIn = True
        self.loopDurationMSec = -1
        self.lastWeight = 0

    def getDurationMSec(self):
        return -1 if self.loop else self.loopDurationMSec

    def getLoopDurationMSec(self):
        return self.loopDurationMSec

    def updateParamExe(self, model, userTimeMSec, weight, userTimeSeconds):
        timeOffset = userTimeMSec - userTimeSeconds.startTimeMSec
        frameFloat = timeOffset * self.srcFps / 1000
        frameIndex = int(frameFloat)
        interpolationFactor = frameFloat - frameIndex
        for i in range(0, len(self.motions), 1):
            motion = self.motions[i]
            valuesLength = len(motion.values)
            paramIdStr = motion.paramIdStr
            if motion.mtnType == Motion.MOTION_TYPE_PARTS_VISIBLE:
                value = motion.values[(valuesLength - 1 if frameIndex >= valuesLength else frameIndex)]
                model.setParamFloat(paramIdStr, value)
            else:
                if Motion.MOTION_TYPE_LAYOUT_X <= motion.mtnType <= Motion.MOTION_TYPE_LAYOUT_SCALE_Y:
                    pass
                else:
                    paramIndex = model.getParamIndex(paramIdStr)
                    modelContext = model.getModelContext()
                    paramMax = modelContext.getParamMax(paramIndex)
                    paramMin = modelContext.getParamMin(paramIndex)
                    thresholdCoeff = 0.4
                    threshold = thresholdCoeff * (paramMax - paramMin)
                    currentParamValue = modelContext.getParamFloat(paramIndex)
                    currentFrameValue = motion.values[(valuesLength - 1 if frameIndex >= valuesLength else frameIndex)]
                    nextFrameValue = motion.values[(valuesLength - 1 if frameIndex + 1 >= valuesLength else frameIndex + 1)]
                    if (currentFrameValue < nextFrameValue and nextFrameValue - currentFrameValue > threshold) or (currentFrameValue > nextFrameValue and currentFrameValue - nextFrameValue > threshold):
                        interpolatedValue = currentFrameValue
                    else:
                        interpolatedValue = currentFrameValue + (nextFrameValue - currentFrameValue) * interpolationFactor

                    newValue = currentParamValue + (interpolatedValue - currentParamValue) * weight
                    model.setParamFloat(paramIdStr, newValue)

        if frameIndex >= self.maxLength:
            if self.loop:
                userTimeSeconds.startTimeMSec = userTimeMSec
                if self.loopFadeIn:
                    userTimeSeconds.fadeInStartTimeMSec = userTimeMSec
            else:
                userTimeSeconds.finished = True

        self.lastWeight = weight

    def isLoop(self):
        return self.loop

    def setLoop(self, aH):
        self.loop = aH

    def isLoopFadeIn(self):
        return self.loopFadeIn

    def setLoopFadeIn(self, value):
        self.loopFadeIn = value

    @staticmethod
    def loadMotion(data: bytes):
        mtn = Live2DMotion()
        pos = [0]
        dataLength = len(data)
        mtn.maxLength = 0
        i = 0
        while i < dataLength:
            byte = data[i]
            char = chr(byte)
            if char == "\n" or char == "\r":
                i += 1
                continue

            if char == "#":
                while i < dataLength:
                    if chr(data[i]) == "\n" or chr(data[i]) == "\r":
                        break
                    i += 1

                i += 1
                continue

            if char == "":
                startPos = i
                equalPos = -1
                while i < dataLength:
                    char = chr(data[i])
                    if char == "\r" or char == "\n":
                        break

                    if char == "=":
                        equalPos = i
                        break
                    i += 1

                isFps = False
                if equalPos >= 0:
                    if equalPos == startPos + 4 and chr(data[startPos + 1]) == "f" and chr(data[startPos + 2]) == "p" and chr(data[startPos + 3]) == "s":
                        isFps = True

                    i = equalPos + 1
                    while i < dataLength:
                        char = chr(data[i])
                        if char == "\r" or char == "\n":
                            break

                        if char == "," or char == " " or char == "\t":
                            i += 1
                            continue

                        value = UtString.strToFloat(data, dataLength, i, pos)
                        if pos[0] > 0:
                            if isFps and 5 < value < 121:
                                mtn.srcFps = value

                        i = pos[0]
                        i += 1

                while i < dataLength:
                    if chr(data[i]) == "\n" or chr(data[i]) == "\r":
                        break
                    i += 1

                i += 1
                continue

            if (97 <= byte <= 122) or (65 <= byte <= 90) or char == "_":
                startPos = i
                equalPos = -1
                while i < dataLength:
                    char = chr(data[i])
                    if char == "\r" or char == "\n":
                        break

                    if char == "=":
                        equalPos = i
                        break
                    i += 1

                if equalPos >= 0:
                    motion = Motion()
                    if UtString.startswith(data, startPos, Live2DMotion.MTN_PREFIX_VISIBLE):
                        motion.mtnType = Motion.MOTION_TYPE_PARTS_VISIBLE
                        motion.paramIdStr = UtString.createString(data, startPos, equalPos - startPos)
                    else:
                        if UtString.startswith(data, startPos, Live2DMotion.MTN_PREFIX_LAYOUT):
                            motion.paramIdStr = UtString.createString(data, startPos + 7, equalPos - startPos - 7)
                            if UtString.startswith(data, startPos + 7, "ANCHOR_X"):
                                motion.mtnType = Motion.MOTION_TYPE_LAYOUT_ANCHOR_X
                            else:
                                if UtString.startswith(data, startPos + 7, "ANCHOR_Y"):
                                    motion.mtnType = Motion.MOTION_TYPE_LAYOUT_ANCHOR_Y
                                else:
                                    if UtString.startswith(data, startPos + 7, "SCALE_X"):
                                        motion.mtnType = Motion.MOTION_TYPE_LAYOUT_SCALE_X
                                    else:
                                        if UtString.startswith(data, startPos + 7, "SCALE_Y"):
                                            motion.mtnType = Motion.MOTION_TYPE_LAYOUT_SCALE_Y
                                        else:
                                            if UtString.startswith(data, startPos + 7, "AffineEnt"):
                                                motion.mtnType = Motion.MOTION_TYPE_LAYOUT_X
                                            else:
                                                if UtString.startswith(data, startPos + 7, "Y"):
                                                    motion.mtnType = Motion.MOTION_TYPE_LAYOUT_Y
                        else:
                            motion.mtnType = Motion.MOTION_TYPE_PARAM
                            motion.paramIdStr = UtString.createString(data, startPos, equalPos - startPos)

                    mtn.motions.append(motion)
                    valueCount = 0
                    values = []
                    i = equalPos + 1
                    while i < dataLength:
                        char = chr(data[i])
                        if char == "\r" or char == "\n":
                            break

                        if char == "," or char == " " or char == "\t":
                            i += 1
                            continue

                        value = UtString.strToFloat(data, dataLength, i, pos)
                        if pos[0] > 0:
                            values.append(value)
                            valueCount += 1
                            newPos = pos[0]
                            if newPos < i:
                                print("invalid state during loadMotion\n")
                                break

                            i = newPos - 1
                        i += 1

                    motion.values = values
                    if valueCount > mtn.maxLength:
                        mtn.maxLength = valueCount

            i += 1

        mtn.loopDurationMSec = int((1000 * mtn.maxLength) / mtn.srcFps)
        return mtn
