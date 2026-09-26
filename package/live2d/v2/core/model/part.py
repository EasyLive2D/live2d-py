from typing import TYPE_CHECKING, Optional, List

from .parts_context import PartsDataContext
from ..io.iserializable import ISerializable
if TYPE_CHECKING:
    from ..draw import Mesh
    from ..id import Id


class PartsData(ISerializable):

    def __init__(self):
        self.visible = True
        self.locked = False
        self.id: Optional['Id'] = None
        self.deformerList = None
        self.drawDataList: List[Mesh] | None = None

    def initDirect(self):
        self.deformerList = []
        self.drawDataList = []

    def read(self, aH):
        self.locked = aH.readBit()
        self.visible = aH.readBit()
        self.id = aH.readObject()
        self.deformerList = aH.readObject()
        self.drawDataList = aH.readObject()

    def init(self):
        aH = PartsDataContext(self)
        aH.setPartsOpacity(1 if self.isVisible() else 0)
        return aH

    def setDeformerList(self, aH):
        self.deformerList = aH

    def setDrawDataList(self, aH):
        self.drawDataList = aH

    def isVisible(self):
        return self.visible

    def isLocked(self):
        return self.locked

    def setVisible(self, aH):
        self.visible = aH

    def setLocked(self, aH):
        self.locked = aH

    def getDeformer(self):
        return self.deformerList

    def getDrawData(self):
        return self.drawDataList

    def getId(self):
        return self.id

    def setId(self, aH):
        self.id = aH
