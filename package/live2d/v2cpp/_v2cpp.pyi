from typing import Any
from ..v2.params import Parameter


class LAppModel:
    """Live2D Cubism 2.x application model."""

    autoBreath: bool
    autoBlink: bool

    def LoadModelJson(self, path: str) -> None:
        """Load model from .model.json file."""
        ...
    def Resize(self, w: int, h: int) -> None:
        """Resize viewport."""
        ...
    def Drag(self, x: float, y: float) -> None:
        """Set drag target point."""
        ...
    def IsMotionFinished(self) -> bool:
        """Check if current motion has finished."""
        ...
    def SetOffset(self, dx: float, dy: float) -> None:
        """Set view offset."""
        ...
    def SetScale(self, scale: float) -> None:
        """Set view scale."""
        ...
    def SetParameterValue(self, id: str, value: float, weight: float = 1.0) -> None:
        """Set parameter value with weight blending."""
        ...
    def AddParameterValue(self, id: str, value: float, weight: float = 1.0) -> None:
        """Add to parameter value with weight blending."""
        ...
    def SetAutoBreathEnable(self, enable: bool) -> None:
        """Enable/disable automatic breath animation."""
        ...
    def SetAutoBlinkEnable(self, enable: bool) -> None:
        """Enable/disable automatic eye blink."""
        ...
    def GetParameterCount(self) -> int:
        """Get total parameter count."""
        ...
    def GetPartCount(self) -> int:
        """Get total part count."""
        ...
    def GetPartId(self, index: int) -> str:
        """Get part ID by index."""
        ...
    def GetPartIds(self) -> list[str]:
        """Get all part IDs."""
        ...
    def SetPartOpacity(self, index: int, opacity: float) -> None:
        """Set part opacity."""
        ...
    def Update(self) -> None:
        """Update model state (motion, physics, pose, parameters)."""
        ...
    def Draw(self) -> None:
        """Draw model to current framebuffer."""
        ...
    def HitTest(self, area: str, x: float, y: float) -> str | None:
        """Hit test against named area."""
        ...
    def SetExpression(self, name: str) -> None:
        """Start an expression motion by name."""
        ...
    def SetRandomExpression(self) -> None:
        """Start a random expression."""
        ...
    def StartMotion(self, group: str, no: int, priority: int) -> None:
        """Start a motion from a group by index and priority."""
        ...
    def StartRandomMotion(self, group: str = "", priority: int = 1) -> None:
        """Start a random motion. If group is empty, picks from all motions."""
        ...
    def GetCanvasWidth(self) -> float:
        """Get model canvas width."""
        ...
    def GetCanvasHeight(self) -> float:
        """Get model canvas height."""
        ...
    def ClearMotions(self) -> None:
        """Stop all running motions."""
        ...
    def ResetExpression(self) -> None:
        """Reset current expression."""
        ...
    def GetParameter(self, index: int) -> Parameter:
        """Get parameter object by index."""
        ...
    def HitPart(self, x: float, y: float, topOnly: bool) -> list[str]:
        """Hit test against drawable parts."""
        ...
    def setPartScreenColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """Set part screen color."""
        ...
    def GetPartScreenColor(self, index: int) -> tuple[float, float, float, float]:
        """Get part screen color."""
        ...
    def SetPartMultiplyColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """Set part multiply color."""
        ...
    def GetPartMultiplyColor(self, index: int) -> tuple[float, float, float, float]:
        """Get part multiply color."""
        ...
    def Rotate(self, deg: float) -> None:
        """Rotate view by degrees."""
        ...
    def GetPixelsPerUnit(self) -> float:
        """Get pixels per unit scale."""
        ...
    def GetCanvasSizePixel(self) -> tuple[float, float]:
        """Get canvas size in pixels."""
        ...


class Live2DLogLevels:
    LV_DEBUG: int = 0
    LV_INFO: int = 0
    LV_WARN: int = 0
    LV_ERROR: int = 0


LIVE2D_VERSION: int = 2


def init() -> None:
    """Initialize platform manager."""
    ...

def glInit() -> None:
    """Initialize OpenGL function pointers."""
    ...

def glRelease() -> None:
    """Release OpenGL resources."""
    ...

def dispose() -> None:
    """Release all live2d resources."""
    ...

def clearBuffer(r: float = 0.0, g: float = 0.0, b: float = 0.0, a: float = 0.0) -> None:
    """Clear color buffer."""
    ...

def enableLog(enable: bool) -> None:
    """Enable/disable log output."""
    ...

def isLogEnabled() -> bool:
    """Check if log is enabled."""
    ...

def setLogLevel(level: int) -> None:
    """Set log level."""
    ...

def getLogLevel() -> int:
    """Get current log level."""
    ...
