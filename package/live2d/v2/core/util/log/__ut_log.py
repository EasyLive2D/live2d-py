import time

RESET = "\033[0m"
RED = "\033[31m"
GREEN = "\033[32m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
MAGENTA = "\033[35m"
CYAN = "\033[36m"
WHITE = "\033[37m"

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
            time.strftime(f"{BLUE}[DEBUG]"),
            *args,
            RESET,
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
            time.strftime(f"{YELLOW}[WARN] "),
            *args,
            RESET,
            **kwargs
        )


def Error(*args, **kwargs):
    if __enable and 3 >= __logLevel:
        print(
            time.strftime(f"{RED}[ERROR]"),
            *args,
            RESET,
            **kwargs
        )
