from typing import Optional, List, TYPE_CHECKING

from ..io.iserializable import ISerializable
from ..param import ParamDefSet

if TYPE_CHECKING:
    from .part import PartsData


class ModelImpl(ISerializable):

    def __init__(self):
        self.paramDefSet: Optional[ParamDefSet] = None
        self.partsDataList: Optional[List['PartsData']] = None
        self.canvasWidth: int = 400
        self.canvasHeight: int = 400

    def initDirect(self):
        if self.paramDefSet is None:
            self.paramDefSet = ParamDefSet()

        if self.partsDataList is None:
            self.partsDataList = []

    def getCanvasWidth(self):
        return self.canvasWidth

    def getCanvasHeight(self):
        return self.canvasHeight

    def read(self, br):
        self.paramDefSet = br.readObject()
        self.partsDataList = br.readObject()
        self.canvasWidth = br.readInt32()
        self.canvasHeight = br.readInt32()

    def getPartsDataList(self):
        return self.partsDataList

    def getParamDefSet(self):
        return self.paramDefSet
