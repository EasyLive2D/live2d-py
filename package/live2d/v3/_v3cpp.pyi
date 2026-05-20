from typing import Any
from .params import Parameter
from typing import Callable


class Live2DLogLevels:
    """
    Live2D log level enumeration
    """
    LV_DEBUG: int = 0  # Debug level
    LV_INFO: int = 0   # Info level
    LV_WARN: int = 0   # Warning level
    LV_ERROR: int = 0  # Error level


def init() -> None:
    """
    Initialize inner memory allocator for live2d models
    """
    ...


def dispose() -> None:
    """
    Dispose Cubism Framework when no longer using live2d
    """
    ...


def glInit() -> None:
    """
    Initialize inner opengl functions
    """
    ...


def glRelease() -> None:
    """
    Release gl shaders when no longer rendering
    
    Should be called when opengl context is active and is about to be destroyed
    """
    ...


def clearBuffer(r=0.0, g=0.0, b=0.0, a=0.0) -> None:
    """
    Clear color buffer
    
    glClearColor(0.0, 0.0, 0.0, 0.0)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
    glClearDepth(1.0)
    
    :param r: Red channel value (0.0-1.0)
    :param g: Green channel value (0.0-1.0)
    :param b: Blue channel value (0.0-1.0)
    :param a: Alpha channel value (0.0-1.0)
    """
    ...


def enableLog(enable: bool) -> None:
    """
    Enable or disable logging
    
    :param enable: Whether to enable logging
    """
    ...


def isLogEnabled() -> bool:
    """
    Check if logging is enabled
    
    :return: Whether logging is enabled
    """
    ...


def setLogLevel(level: int) -> None:
    """
    Set log level
    
    :param level: Log level, use constants from Live2DLogLevels
    """
    ...


def getLogLevel() -> int:
    """
    Get current log level
    
    :return: Current log level
    """
    ...


class Model:
    """
    Model class provides low-level control interface for Live2D models,
    capable of loading models, updating states, controlling animations, handling interactions, etc.
    """

    def __init__(self) -> None:
        """
        Initialize Model instance
        """
        ...

    def LoadModelJson(self, modelJson: str) -> None:
        """
        Load model from JSON file
        
        :param modelJson: Path to model JSON file
        """
        ...

    def GetModelHomeDir(self) -> str:
        """
        Get model directory path
        
        :return: Model directory path
        """
        ...

    def Update(self, deltaTimeSeconds: float) -> bool:
        """
        Update overall model state
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        :return: Whether update was successful
        """
        ...

    def UpdateMotion(self, deltaTimeSeconds: float) -> bool:
        """
        Update model motion state
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        :return: Whether update was successful
        """
        ...

    def UpdateDrag(self, deltaTimeSeconds: float) -> None:
        """
        Update drag state
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        """
        ...

    def UpdateBreath(self, deltaTimeSeconds: float) -> None:
        """
        Update breath effect
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        """
        ...

    def UpdateBlink(self, deltaTimeSeconds: float) -> None:
        """
        Update blink effect
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        """
        ...

    def UpdateExpression(self, deltaTimeSeconds: float) -> None:
        """
        Update expression
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        """
        ...

    def UpdatePhysics(self, deltaTimeSeconds: float) -> None:
        """
        Update physics simulation
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        """
        ...

    def UpdatePose(self, deltaTimeSeconds: float) -> None:
        """
        Update pose
        
        :param deltaTimeSeconds: Time interval since last update (in seconds)
        """
        ...

    def GetParameterIds(self) -> list[str]:
        """
        Get list of IDs for all parameters
        
        :return: List of parameter IDs
        """
        ...

    def GetParameterValue(self, index: int) -> float:
        """
        Get parameter value by index
        
        :param index: Parameter index
        :return: Parameter value
        """
        ...

    def GetParameterMaximumValue(self, index: int) -> float:
        """
        Get maximum value for specified parameter
        
        :param index: Parameter index
        :return: Maximum parameter value
        """
        ...

    def GetParameterMinimumValue(self, index: int) -> float:
        """
        Get minimum value for specified parameter
        
        :param index: Parameter index
        :return: Minimum parameter value
        """
        ...

    def GetParameterDefaultValue(self, index: int) -> float:
        """
        Get default value for specified parameter
        
        :param index: Parameter index
        :return: Default parameter value
        """
        ...

    def SetParameterValue(self, index: int, value: float, weight: float = 1.0) -> None:
        """
        Set parameter value by index
        
        :param index: Parameter index
        :param value: Parameter value
        :param weight: Weight value, defaults to 1.0
        """
        ...

    def SetParameterValueById(self, paramId: str, value: float, weight: float = 1.0) -> None:
        """
        Set parameter value by parameter ID
        
        :param paramId: Parameter ID
        :param value: Parameter value
        :param weight: Weight value, defaults to 1.0
        """
        ...

    def AddParameterValue(self, index: int, value: float) -> None:
        """
        Add value to parameter by index
        
        :param index: Parameter index
        :param value: Value to add
        """
        ...

    def AddParameterValueById(self, paramId: str, value: float) -> None:
        """
        Add value to parameter by parameter ID
        
        :param paramId: Parameter ID
        :param value: Value to add
        """
        ...

    def SetAndSaveParameterValue(self, index: int, value: float, weight: float = 1.0) -> None:
        """
        Set parameter value and save to model parameters
        
        :param index: Parameter index
        :param value: Parameter value
        :param weight: Weight value, defaults to 1.0
        """
        ...

    def SetAndSaveParameterValueById(self, paramId: str, value: float, weight: float = 1.0) -> None:
        """
        Set parameter value by parameter ID and save to model parameters
        
        :param paramId: Parameter ID
        :param value: Parameter value
        :param weight: Weight value, defaults to 1.0
        """
        ...

    def AddAndSaveParameterValue(self, index: int, value: float) -> None:
        """
        Add value to parameter by index and save to model parameters
        
        :param index: Parameter index
        :param value: Value to add
        """
        ...

    def AddAndSaveParameterValueById(self, paramId: str, value: float) -> None:
        """
        Add value to parameter by parameter ID and save to model parameters
        
        :param paramId: Parameter ID
        :param value: Value to add
        """
        ...

    def LoadParameters(self) -> None:
        """
        Load parameters from model
        """
        ...

    def SaveParameters(self) -> None:
        """
        Save parameters to model
        """
        ...

    def Resize(self, width: int, height: int) -> None:
        """
        Adjust model display size
        
        :param width: New width
        :param height: New height
        """
        ...

    def SetOffset(self, x: float, y: float) -> None:
        """
        Set model offset
        
        :param x: X-axis offset
        :param y: Y-axis offset
        """
        ...

    def Rotate(self, degrees: float) -> None:
        """
        Rotate model
        
        :param degrees: Rotation angle
        """
        ...

    def SetScale(self, scale: float) -> None:
        """
        Set model scale factor
        
        :param scale: Scale factor
        """
        ...

    def GetMvp(self) -> list[float]:
        """
        Get model-view-projection matrix
        
        :return: MVP matrix elements list
        """
        ...

    def StartMotion(self, group: str, no: int, priority: int = 3, 
                    onStart: Callable[[str, int], None] = None, 
                    onFinish: Callable[[str, int], None] = None) -> None:
        """
        Start playing specified motion
        
        :param group: Motion group name
        :param no: Motion number
        :param priority: Priority
        :param onStart: Callback function when playback starts
        :param onFinish: Callback function when playback completes
        """
        ...

    def StartRandomMotion(self, group: str = None, priority: int = 3, 
                          onStart: Callable[[str, int], None] = None, 
                          onFinish: Callable[[str, int], None] = None) -> None:
        """
        Randomly play a motion
        
        :param group: Motion group name
        :param priority: Priority
        :param onStart: Callback function when playback starts
        :param onFinish: Callback function when playback completes
        """
        ...

    def IsMotionFinished(self) -> bool:
        """
        Check if current motion has finished playing
        
        :return: Returns True if motion is finished, otherwise False
        """
        ...

    def LoadExtraMotion(self, group: str, motionJsonPath: str) -> int:
        """
        Load an additional motion from a JSON file and add it to the specified motion group.
        This allows dynamically adding new motions to the model beyond those initially loaded.
        
        :param group: Motion group name to add the motion to
        :param motionJsonPath: Path to the motion JSON file to load

        :return: Returns the number of motions loaded, or -1 if an error occurs
        """
        ...

    def GetMotions(self) -> dict[str, list[dict[str, str]]]:
        """
        Get all motion information defined in model3.json
        
        :return: Dictionary containing all motion information
        """
        ...

    def HitPart(self, x: float, y: float, topOnly: bool = False) -> list[str]:
        """
        Detect parts at specified coordinate
        
        :param x: X coordinate
        :param y: Y coordinate
        :param topOnly: Whether to return only the topmost part
        :return: List of part IDs
        """
        ...

    def HitDrawable(self, x: float, y: float, topOnly: bool = False) -> list[str]:
        """
        Detect drawable objects at specified coordinate
        
        :param x: X coordinate
        :param y: Y coordinate
        :param topOnly: Whether to return only the topmost object
        :return: List of drawable object IDs
        """
        ...

    def Drag(self, x: float, y: float) -> None:
        """
        Perform drag operation
        
        :param x: X coordinate
        :param y: Y coordinate
        """
        ...

    def IsAreaHit(self, areaName: str, x: float, y: float) -> bool:
        """
        Check if specified area is clicked
        
        :param areaName: Area name
        :param x: X coordinate
        :param y: Y coordinate
        :return: Returns True if area is clicked, otherwise False
        """
        ...

    def IsPartHit(self, index: int, x: float, y: float) -> bool:
        """
        Check if specified part is clicked
        
        :param index: Part index
        :param x: X coordinate
        :param y: Y coordinate
        :return: Returns True if part is clicked, otherwise False
        """
        ...

    def IsDrawableHit(self, index: int, x: float, y: float) -> bool:
        """
        Check if specified drawable object is clicked
        
        :param index: Drawable object index
        :param x: X coordinate
        :param y: Y coordinate
        :return: Returns True if object is clicked, otherwise False
        """
        ...

    def CreateRenderer(self, maskBufferCount: int = 1) -> None:
        """
        Create renderer
        
        :param maskBufferCount: Number of mask buffers, defaults to 1
        """
        ...

    def DestroyRenderer(self) -> None:
        """
        Destroy renderer, release textures and buffers
        
        Should be called when opengl context is active and is about to be destroyed
        """
        ...

    def Draw(self) -> None:
        """
        Render model
        """
        ...

    def GetPartIds(self) -> list[str]:
        """
        Get list of all part IDs
        
        :return: List of part IDs
        """
        ...

    def SetPartOpacity(self, index: int, opacity: float) -> None:
        """
        Set part opacity
        
        :param index: Part index
        :param opacity: Opacity value (0.0-1.0)
        """
        ...

    def SetPartScreenColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """
        Set screen blend color for part
        
        :param index: Part index
        :param r: Red value (0.0-1.0)
        :param g: Green value (0.0-1.0)
        :param b: Blue value (0.0-1.0)
        :param a: Alpha value (0.0-1.0)
        """
        ...

    def SetPartMultiplyColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """
        Set multiply color for part
        
        :param index: Part index
        :param r: Red value (0.0-1.0)
        :param g: Green value (0.0-1.0)
        :param b: Blue value (0.0-1.0)
        :param a: Alpha value (0.0-1.0)
        """
        ...

    def GetDrawableIds(self) -> list[str]:
        """
        Get list of all drawable object IDs
        
        :return: List of drawable object IDs
        """
        ...

    def SetDrawableMultiplyColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """
        Set multiply color for drawable object
        
        :param index: Drawable object index
        :param r: Red value (0.0-1.0)
        :param g: Green value (0.0-1.0)
        :param b: Blue value (0.0-1.0)
        :param a: Alpha value (0.0-1.0)
        """
        ...

    def SetDrawableScreenColor(self, index: int, r: float, g: float, b: float, a: float) -> None:
        """
        Set screen blend color for drawable object
        
        :param index: Drawable object index
        :param r: Red value (0.0-1.0)
        :param g: Green value (0.0-1.0)
        :param b: Blue value (0.0-1.0)
        :param a: Alpha value (0.0-1.0)
        """
        ...

    def AddExpression(self, expressionId: str) -> None:
        """
        Add expression
        
        :param expressionId: Expression ID
        """
        ...

    def RemoveExpression(self, expressionId: str) -> None:
        """
        Remove expression
        
        :param expressionId: Expression ID
        """
        ...

    def ResetExpression(self) -> None:
        """
        Reset to default expression
        """
        ...

    def ResetExpressions(self) -> None:
        """
        Reset all expressions
        """
        ...

    def GetExpressions(self) -> list[str, str]:
        """
        Get all expression information defined in model3.json
        
        :return: List of expression information
        """
        ...

    def SetExpression(self, expressionId: str) -> None:
        """
        Set expression
        
        :param expressionId: Expression ID
        """
        ...

    def SetRandomExpression(self) -> str:
        """
        Set random expression
        
        :return: Set expression ID
        """
        ...
    
    def LoadExtraExpression(self, expressionId: str, path: str) -> None:
        """
        Load extra expressions
        
        :param expressionId: Expression ID
        :param path: Path to expression file
        """

    def StopAllMotions(self) -> None:
        """
        Stop all motions
        """
        ...

    def ResetAllParameters(self) -> None:
        """
        Reset all parameters
        """
        ...

    def ResetPose(self) -> None:
        """
        Reset pose
        """
        ...

    def GetCanvasSize(self) -> tuple[float, float]:
        """
        Get canvas size
        
        :return: Canvas size tuple (width, height)
        """
        ...

    def GetCanvasSizePixel(self) -> tuple[float, float]:
        """
        Get canvas pixel size
        
        :return: Canvas pixel size tuple (width, height)
        """
        ...

    def GetPixelsPerUnit(self) -> float:
        """
        Get pixels per unit
        
        :return: Pixels per unit
        """
        ...

    def SetOffsetX(self, sx: float) -> None:
        """
        Set X-axis offset
        
        :param sx: X-axis offset
        """
        ...

    def SetOffsetY(self, sy: float) -> None:
        """
        Set Y-axis offset
        
        :param sy: Y-axis offset
        """
        ...

    def SetAutoBreath(self, on: bool) -> None:
        """
        Set auto breath feature toggle
        
        :param on: Whether to enable auto breath
        """
        ...

    def SetAutoBlink(self, on: bool) -> None:
        """
        Set auto blink feature toggle
        
        :param on: Whether to enable auto blink
        """
        ...