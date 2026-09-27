from typing import Any, Callable
from ..v2.params import Parameter


class LAppModel:
    """Live2D Cubism 2.x application model (v2 API + v3 fine-grained API)."""

    autoBreath: bool
    autoBlink: bool

    # ---- v2 API ----

    def LoadModelJson(self, path: str, create_renderer: bool = True) -> None:
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
    def Update(self, deltaSecs: float = -1.0) -> None:
        """Update model state (motion, physics, pose, parameters).
        deltaSecs < 0（默认）: 墙钟自适配（Python v2 1:1）；deltaSecs >= 0: delta 驱动
        （可确定性测试/变速/暂停）。"""
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
    def StartMotion(self, group: str, no: int, priority: int,
                    onStart: Callable[[str, int], None] | None = None,
                    onFinish: Callable[[str, int], None] | None = None) -> None:
        """Start a motion from a group by index and priority.
        Callbacks accept (group, no)."""
        ...
    def StartRandomMotion(self, group: str = "", priority: int = 3,
                          onStart: Callable[[str, int], None] | None = None,
                          onFinish: Callable[[str, int], None] | None = None) -> None:
        """Start a random motion. If group is empty, picks from all motions."""
        ...
    def GetCanvasWidth(self) -> float:
        """Get model canvas width."""
        ...
    def GetCanvasHeight(self) -> float:
        """Get model canvas height."""
        ...
    def GetCanvasSize(self) -> tuple[float, float]:
        """Get model canvas size."""
        ...
    def ClearMotions(self) -> None:
        """Stop all running motions."""
        ...
    def StopAllMotions(self) -> None:
        """Stop all running motions."""
        ...
    def ResetExpression(self) -> None:
        """Reset current expression."""
        ...
    def ResetPose(self) -> None:
        """Reset pose."""
        ...
    def GetParameter(self, index: int) -> Parameter:
        """Get parameter object by index."""
        ...
    def HitPart(self, x: float, y: float, topOnly: bool = False) -> list[str]:
        """Hit test against drawable parts."""
        ...
    def SetPartScreenColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """Set part screen color."""
        ...
    def setPartScreenColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """Set part screen color (alias)."""
        ...
    def GetPartScreenColor(self, index: int) -> list[float]:
        """Get part screen color."""
        ...
    def SetPartMultiplyColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """Set part multiply color."""
        ...
    def GetPartMultiplyColor(self, index: int) -> list[float]:
        """Get part multiply color."""
        ...
    def Rotate(self, deg: float) -> None:
        """Rotate view by degrees."""
        ...
    def GetPixelsPerUnit(self) -> int:
        """Get pixels per unit scale."""
        ...
    def GetCanvasSizePixel(self) -> tuple[float, float]:
        """Get canvas size in pixels."""
        ...
    def CreateRenderer(self, maskBufferCount: int = 1) -> Any:
        """Create a renderer for the model."""
        ...
    def ReleaseRenderer(self) -> None:
        """Release the renderer."""
        ...

    # ---- v3 fine-grained API (aligned with live2d.v3.Model) ----

    def Version(self) -> int:
        """Get SDK version (2 for v2, 3 for v3)."""
        ...
    def IsV2(self) -> bool:
        """Check if model is Cubism 2.x."""
        ...
    def IsV3(self) -> bool:
        """Check if model is Cubism 3.x."""
        ...
    def GetModelHomeDir(self) -> str:
        """Get model home directory."""
        ...
    def GetParameterIds(self) -> list[str]:
        """Get all parameter IDs."""
        ...
    def GetDrawableIds(self) -> list[str]:
        """Get all drawable IDs."""
        ...
    def GetExpressions(self) -> list[str]:
        """Get all expression IDs."""
        ...
    def AddExpression(self, id: str) -> None:
        """Start an expression (v2: equivalent to SetExpression)."""
        ...
    def RemoveExpression(self, id: str) -> None:
        """Stop expressions (v2: single expression manager, stops all)."""
        ...
    def ResetExpressions(self) -> None:
        """Reset all expressions."""
        ...
    def LoadExtraExpression(self, id: str, path: str) -> None:
        """Load an extra expression file."""
        ...
    def GetMotions(self) -> dict[str, list[dict[str, str]]]:
        """Get all motions: {group: [{"File": ..., "Sound": ...}, ...]}."""
        ...
    def LoadExtraMotion(self, group: str, path: str) -> int:
        """Load an extra motion file into a group, returns motion index."""
        ...
    def IsAreaHit(self, area: str, x: float, y: float) -> bool:
        """Check if named hit area is hit."""
        ...
    def HitDrawable(self, x: float, y: float, topOnly: bool = False) -> list[str]:
        """Hit test against drawables."""
        ...
    def LoadParameters(self) -> None:
        """Restore parameter values from saved state."""
        ...
    def SaveParameters(self) -> None:
        """Save current parameter values."""
        ...
    def UpdateMotion(self, deltaSecs: float) -> bool:
        """Update motion (v2: wall-clock driven). Returns True if updated."""
        ...
    def UpdateDrag(self, deltaSecs: float) -> None:
        """Update drag parameter animation."""
        ...
    def UpdateBreath(self, deltaSecs: float) -> None:
        """Update breath animation (v2: wall-clock driven)."""
        ...
    def UpdateBlink(self, deltaSecs: float) -> None:
        """Update eye blink."""
        ...
    def UpdateExpression(self, deltaSecs: float) -> None:
        """Update expression motion."""
        ...
    def UpdatePhysics(self, deltaSecs: float) -> None:
        """Update physics."""
        ...
    def UpdatePose(self, deltaSecs: float) -> None:
        """Update pose."""
        ...


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
