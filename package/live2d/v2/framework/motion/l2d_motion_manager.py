from ...core import MotionQueueManager


class L2DMotionManager(MotionQueueManager):

    def __init__(self):
        super().__init__()
        self.currentPriority = 0
        self.reservePriority = 0

    def getCurrentPriority(self):
        return self.currentPriority

    def getReservePriority(self):
        return self.reservePriority

    def reserveMotion(self, priority):
        if self.reservePriority > priority:
            return False

        if self.currentPriority > priority:
            return False

        self.reservePriority = priority
        return True

    def setReservePriority(self, val):
        self.reservePriority = val

    def updateParam(self, model):
        updated = super().updateParam(model)
        if self.isFinished():
            self.currentPriority = 0

        return updated

    def startMotionPrio(self, motion, priority):
        if priority == self.reservePriority:
            self.reservePriority = 0

        self.currentPriority = priority
        return self.startMotion(motion, False)
