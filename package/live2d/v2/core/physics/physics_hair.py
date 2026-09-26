import math

from ..util import UtMath
from .physics_constants import *
from .physics_point import PhysicsPoint
from .physics_src import PhysicsSrc
from .physics_target import PhysicsTarget


class PhysicsHair:
    SRC_TO_X = SRC_TO_X
    SRC_TO_Y = SRC_TO_Y
    SRC_TO_G_ANGLE = SRC_TO_G_ANGLE

    TARGET_FROM_ANGLE = TARGET_FROM_ANGLE
    TARGET_FROM_ANGLE_V = TARGET_FROM_ANGLE_V

    def __init__(self):
        self.p1 = PhysicsPoint()
        self.p2 = PhysicsPoint()
        self.length = 0
        self.angle = 0
        self.stiffness = 0
        self.lastAngle = 0
        self.currentAngle = 0
        self.angleVelocity = 0
        self.lastTime = 0
        self.currentTime = 0
        self.sourceParams = []
        self.targetParams = []
        self.setup(0.3, 0.5, 0.1)

    def setup(self, length=None, stiffness=None, mass=None):
        self.currentAngle = self.calcAngle()
        self.p2.setupLast()
        if mass is not None:
            self.length = length
            self.stiffness = stiffness
            self.p1.mass = mass
            self.p2.mass = mass
            self.p2.y = length

    def getPhysicsPoint1(self):
        return self.p1

    def getPhysicsPoint2(self):
        return self.p2

    def getAngle(self):
        return self.angle

    def setAngle(self, angle):
        self.angle = angle

    def getLastAngle(self):
        return self.lastAngle

    def getLastAngleVelocity(self):
        return self.angleVelocity

    def calcAngle(self):
        return -180 * (math.atan2(self.p1.x - self.p2.x, -(self.p1.y - self.p2.y))) / math.pi

    def addSrcParam(self, sourceType, paramId, scale, weight):
        srcParam = PhysicsSrc(sourceType, paramId, scale, weight)
        self.sourceParams.append(srcParam)

    def addTargetParam(self, targetType, paramId, scale, weight):
        targetParam = PhysicsTarget(targetType, paramId, scale, weight)
        self.targetParams.append(targetParam)

    def update(self, model, timeMSec):
        if self.lastTime == 0:
            self.lastTime = self.currentTime = timeMSec
            self.length = (math.sqrt((self.p1.x - self.p2.x) * (self.p1.x - self.p2.x) + (self.p1.y - self.p2.y) * (
                    self.p1.y - self.p2.y)))
            return

        deltaTime = (timeMSec - self.currentTime) / 1000
        if deltaTime != 0:
            for i in range(len(self.sourceParams) - 1, 0 - 1, -1):
                srcParam = self.sourceParams[i]
                srcParam.update(model, self)

            self.updatePhysics(model, deltaTime)
            self.lastAngle = self.calcAngle()
            self.angleVelocity = (self.lastAngle - self.currentAngle) / deltaTime
            self.currentAngle = self.lastAngle

        for i in range(len(self.targetParams) - 1, 0 - 1, -1):
            targetParam = self.targetParams[i]
            targetParam.update(model, self)

        self.currentTime = timeMSec

    def updatePhysics(self, model, deltaTime):
        if deltaTime < 0.033:
            deltaTime = 0.033

        invTime = 1 / deltaTime
        self.p1.vx = (self.p1.x - self.p1.lastX) * invTime
        self.p1.vy = (self.p1.y - self.p1.lastY) * invTime
        self.p1.ax = (self.p1.vx - self.p1.lastVX) * invTime
        self.p1.ay = (self.p1.vy - self.p1.lastVY) * invTime
        self.p1.fx = self.p1.ax * self.p1.mass
        self.p1.fy = self.p1.ay * self.p1.mass
        self.p1.setupLast()
        angle = -(math.atan2((self.p1.y - self.p2.y), self.p1.x - self.p2.x))
        cosAngle = math.cos(angle)
        sinAngle = math.sin(angle)
        gravity = 9.8 * self.p2.mass
        angleRad = (self.angle * UtMath.DEG_TO_RAD)
        forceGravity = (gravity * math.cos(angle - angleRad))
        forceX = (forceGravity * sinAngle)
        forceY = (forceGravity * cosAngle)
        forceDragX = (-self.p1.fx * sinAngle * sinAngle)
        forceDragY = (-self.p1.fy * sinAngle * cosAngle)
        forceStiffnessX = (-self.p2.vx * self.stiffness)
        forceStiffnessY = (-self.p2.vy * self.stiffness)
        self.p2.fx = (forceX + forceDragX + forceStiffnessX)
        self.p2.fy = (forceY + forceDragY + forceStiffnessY)
        self.p2.ax = self.p2.fx / self.p2.mass
        self.p2.ay = self.p2.fy / self.p2.mass
        self.p2.vx += self.p2.ax * deltaTime
        self.p2.vy += self.p2.ay * deltaTime
        self.p2.x += self.p2.vx * deltaTime
        self.p2.y += self.p2.vy * deltaTime
        distance = (math.sqrt(
            (self.p1.x - self.p2.x) * (self.p1.x - self.p2.x) + (self.p1.y - self.p2.y) * (self.p1.y - self.p2.y)))
        self.p2.x = self.p1.x + self.length * (self.p2.x - self.p1.x) / distance
        self.p2.y = self.p1.y + self.length * (self.p2.y - self.p1.y) / distance
        self.p2.vx = (self.p2.x - self.p2.lastX) * invTime
        self.p2.vy = (self.p2.y - self.p2.lastY) * invTime
        self.p2.setupLast()
