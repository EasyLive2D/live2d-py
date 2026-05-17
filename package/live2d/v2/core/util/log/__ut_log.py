import time

__enable = True

__logLevel = 0


def enableLog(v: bool):
    global __enable
    __enable = v


def isLogEnabled() -> bool:
    return __enable


def setLogLevel(level: int):
    global __logLevel
    __logLevel = level
    match __logLevel:
        case 0:
            Debug("[Log] Level=DEBUG")
        case 1:
            Info("[Log] Level=INFO")
        case 2:
            Warn("[Log] Level=WARN")
        case 3:
            Error("[Log] Level=ERROR")    


def getLogLevel() -> int:
    return __logLevel 


def Debug(*args, **kwargs):
    if __enable and 0 >= __logLevel:
        print(
            time.strftime(f"[DEBUG]"),
            *args,
            **kwargs
        )


def Info(*args, **kwargs):
    if __enable and 1 >= __logLevel:
        print(
            time.strftime("[INFO] "),
            *args,
            **kwargs
        )


def Warn(*args, **kwargs):
    if __enable and 2 >= __logLevel:
        print(
            time.strftime(f"[WARN] "),
            *args,
            **kwargs
        )


def Error(*args, **kwargs):
    if __enable and 3 >= __logLevel:
        print(
            time.strftime(f"[ERROR]"),
            *args,
            **kwargs
        )
