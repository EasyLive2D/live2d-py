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


class LAppModel:
    """
    The LAppModel class provides a structured way to interact with Live2D models, 
    enabling you to load assets, update the model per frame, manage motions, set 
    expressions, and perform hit testing.
    """

    def __init__(self):
        """
        Initialize LAppModel instance
        """
        ...

    def LoadModelJson(self, modelJsonPath: str | Any, maskBufferCount: int = 2) -> None:
        """
        Load Live2D model assets
        
        :param modelJsonPath: Path to the model's JSON configuration file
        :param maskBufferCount: Number of mask buffers, defaults to 2
        """
        ...

    def Resize(self, ww: int | Any, wh: int | Any) -> None:
        """
        Adjust model canvas to window size
        
        :param ww: Window width
        :param wh: Window height
        """
        ...

    def Draw(self) -> None:
        """
        Update model shapes with the params set by `LAppModel.Update` and  `LAppModel.SetParameterValue`, and then render them
        """
        ...

    def StartMotion(self, group: str | Any, no: int | Any, priority: int | Any, onStartMotionHandler=None,
                    onFinishMotionHandler=None) -> None:
        """
        Start a specific motion for the model.
        
        :param group: The group name of the motion.
        :param no: The motion number within the group.
        :param priority: Priority of the motion. Higher priority motions can interrupt lower priority ones.
        :param onStartMotionHandler: Optional callback function that gets called when the motion starts.
        :param onFinishMotionHandler: Optional callback function that gets called when the motion finishes.
        """
        ...

    def StartRandomMotion(self, group: str | Any = None, priority: int | Any = 3, onStartMotionHandler=None,
                          onFinishMotionHandler=None) -> None:
        """
        Start a random motion from a specified group.
        
        :param group: The group name of the motion.
        :param priority: Priority of the motion. Higher priority motions can interrupt lower priority ones.
        :param onStartMotionHandler: Optional callback function that gets called when the motion starts.
        :param onFinishMotionHandler: Optional callback function that gets called when the motion finishes.
        """
        ...

    def SetExpression(self, expressionID: str | Any, fadeout=-1) -> None:
        """
        Set a specific expression for the model.
        
        :param expressionID: name of the expression to be set.
        :param fadeout: Expression fade-out time, -1 means using default time
        """
        ...

    def SetRandomExpression(self, fadeout=-1) -> str:
        """
        Set a random expression for the model.
        
        :param fadeout: Expression fade-out time, -1 means using default time
        :return: Set expression ID
        """
        ...

    def HitTest(self, hitAreaName: str, x: float | Any, y: float | Any) -> bool:
        """
        Test if the current clicked area is the target `HitArea` defined in xxx.model3.json

        x, y are relative to the window topleft
        
        :param hitAreaName: Name of the hit area
        :param x: X coordinate (relative to top-left of window)
        :param y: Y coordinate (relative to top-left of window)
        :return: Returns True if hit, otherwise False
        """
        ...

    def HasMocConsistencyFromFile(self, mocFileName: str | Any) -> bool:
        """
        Check if the model's MOC file is consistent.
        
        :param mocFileName: Name of the MOC file to check.
        :return: True if the MOC file is consistent, otherwise False.
        """
        ...

    def Drag(self, x: float | Any, y: float | Any) -> None:
        """
        Perform drag operation
        
        :param x: Global mouse X coordinate - window X coordinate
        :param y: Global mouse Y coordinate - window Y coordinate
        """
        ...

    def IsMotionFinished(self) -> bool:
        """
        Check if the currently playing motion has finished
        
        :return: Returns True if motion is completed, otherwise False
        """
        ...

    def SetOffset(self, dx: float | Any, dy: float | Any) -> None:
        """
        Set offset of model center coordinates
        
        :param dx: X-axis offset
        :param dy: Y-axis offset
        """
        ...

    def SetScale(self, scale: float | Any) -> None:
        """
        Set model scale factor
        
        :param scale: Scale factor
        """
        ...

    def Rotate(self, degrees: float) -> None:
        """
        Rotate the model
        
        :param degrees: Rotation angle, counterclockwise when degrees > 0
        """
        ...

    def SetParameterValue(self, paramId: str, value: float, weight: float = 1.0) -> None:
        """
        Set specific value for corresponding control parameter
        Call timing: After CalcParameters, before Update
        
        :param paramId: Parameters embedded in the live2d model, see live2d.v3.params.StandardParams for details
        :param value: All controllable parameters can be found in the official documentation: https://docs.live2d.com/en/cubism-editor-manual/standard-parameter-list/
        :param weight: Ratio of the current input value to the original value, final value = original_value*(1-weight) + input_value*weight
        """
        ...

    def SetIndexParamValue(self, index: int, value: float, weight: float = 1.0) -> None:
        """
        Set parameter value by index
        
        :param index: Parameter index
        :param value: Parameter value
        :param weight: Weight value, defaults to 1.0
        """
        ...

    def AddParameterValue(self, paramId: str, value: float) -> None:
        """
        Add value to specified parameter (final value = original_value + value)
        
        :param paramId: Parameter ID
        :param value: Value to add
        """
        ...

    def AddIndexParamValue(self, index: int, value: float) -> None:
        """
        Add value to parameter by index
        
        :param index: Parameter index
        :param value: Value to add
        """
        ...

    def Update(self) -> None:
        """
        Update model, initialize necessary parameter values such as breathing, motion, pose, expression, transparency of parts, etc.
        """
        ...

    def SetAutoBreathEnable(self, enable: bool) -> None:
        """
        Enable or disable auto breathing function
        
        :param enable: Whether to enable auto breathing
        """
        ...

    def SetAutoBlinkEnable(self, enable: bool) -> None:
        """
        Enable or disable auto blinking function
        
        :param enable: Whether to enable auto blinking
        """
        ...

    def GetParameterCount(self) -> int:
        """
        Get total number of model parameters
        
        :return: Total number of parameters
        """
        ...

    def GetParameter(self, index: int) -> Parameter:
        """
        Get parameter object by index
        
        :param index: Parameter index
        :return: Parameter object
        """
        ...
    
    def GetParamIds(self) -> list[str]:
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

    def GetPartCount(self) -> int:
        """
        Get total number of model parts
        
        :return: Total number of parts
        """
        ...

    def GetPartId(self, index: int) -> str:
        """
        Get part ID by index
        
        :param index: Part index
        :return: Part ID
        """
        ...

    def GetPartIds(self) -> list[str]:
        """
        Get list of IDs for all parts
        
        :return: List of part IDs
        """
        ...

    def SetPartOpacity(self, index: int, opacity: float) -> None:
        """
        Set opacity of specified part
        
        :param index: Part index
        :param opacity: Opacity value (0.0-1.0)
        """
        ...

    def HitPart(self, x: float, y: float, topOnly: bool = False) -> list[str]:
        """
        Detect parts at specified coordinate position
        
        :param x: Screen coordinate x
        :param y: Screen coordinate y
        :param topOnly: Whether to return only the topmost part id
        :return: List of part ids
        """
        ...

    def SetPartScreenColor(self, partIndex: int, r: float, g: float, b: float, a: float) -> None:
        """
        Set screen blend color for specified part
        
        :param partIndex: Part index
        :param r: Red value (0.0-1.0)
        :param g: Green value (0.0-1.0)
        :param b: Blue value (0.0-1.0)
        :param a: Alpha value (0.0-1.0)
        """
        ...
    
    def GetPartScreenColor(self, partIndex: int) -> tuple[float]:
        """
        Get screen blend color for specified part
        
        :param partIndex: Part index
        :return: Color value tuple (r, g, b, a)
        """
        ...

    def SetPartMultiplyColor(self, partIndex: int, r: float, g: float, b: float, a: float) -> None:
        """
        Set multiply color for specified part
        
        :param partIndex: Part index
        :param r: Red value (0.0-1.0)
        :param g: Green value (0.0-1.0)
        :param b: Blue value (0.0-1.0)
        :param a: Alpha value (0.0-1.0)
        """
        ...
    
    def GetPartMultiplyColor(self, partIndex: int) -> tuple[float]:
        """
        Get multiply color value for specified part
        
        :param partIndex: Part index
        :return: Color value tuple (r, g, b, a)
        """
        ...
    
    def GetDrawableIds(self) -> list[str]:
        """
        Get list of drawable object IDs
        
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

    def ResetExpression(self) -> None:
        """
        Reset to default expression
        """
        ...

    def StopAllMotions(self) -> None:
        """
        Stop all ongoing motions
        """
        ...

    def ResetParameters(self) -> None:
        """
        Reset all parameters to default values
        """
        ...

    def ResetPose(self) -> None:
        """
        Reset model pose
        """
        ...

    def GetExpressionIds(self) -> list[str]:
        """
        Get list of all expression IDs
        
        :return: List of expression IDs
        """
        ...

    def GetMotionGroups(self) -> dict[str, int]:
        """
        Get all motion group information
        
        :return: Motion group dictionary, key is group name, value is motion count
        """
        ...

    def GetSoundPath(self, group: str, index: int) -> str:
        """
        Get sound file path for specified motion
        
        :param group: Motion group name
        :param index: Motion index
        :return: Sound file path
        """
        ...
    
    def GetCanvasSize(self) -> tuple[float, float]:
        """
        Get model canvas size
        
        :return: Canvas size tuple (width, height)
        """
        ...

    def GetCanvasSizePixel(self) -> tuple[float, float]:
        """
        Get model canvas pixel size
        
        :return: Canvas pixel size tuple (width, height)
        """
        ...

    def GetPixelsPerUnit(self) -> float:
        """
        Get pixels per unit length
        
        :return: Pixels per unit length
        """
        ...

    def AddExpression(self, expId: str) -> None:
        """
        Add expression
        
        :param expId: Expression ID
        """
        ...

    def RemoveExpression(self, expId: str) -> None:
        """
        Remove specified expression
        
        :param expId: Expression ID
        """
        ...

    def ResetExpressions(self) -> None:
        """
        Reset all expressions
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