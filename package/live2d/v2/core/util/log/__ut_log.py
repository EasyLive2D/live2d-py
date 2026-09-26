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
            LOGD("[Log] Level=DEBUG")
        case 1:
            LOGI("[Log] Level=INFO")
        case 2:
            LOGW("[Log] Level=WARN")
        case 3:
            LOGE("[Log] Level=ERROR")    


def getLogLevel() -> int:
    return __logLevel 


def LOGD(*args, **kwargs):
    if __enable and 0 >= __logLevel:
        print(
            time.strftime(f"[D] "),
            *args,
            **kwargs
        )


def LOGI(*args, **kwargs):
    if __enable and 1 >= __logLevel:
        print(
            time.strftime("[I] "),
            *args,
            **kwargs
        )


def LOGW(*args, **kwargs):
    if __enable and 2 >= __logLevel:
        print(
            time.strftime(f"[W] "),
            *args,
            **kwargs
        )


def LOGE(*args, **kwargs):
    if __enable and 3 >= __logLevel:
        print(
            time.strftime(f"[E] "),
            *args,
            **kwargs
        )
