import time

def LOGD(*args, **kwargs):
    print(
        time.strftime(f"[D] "),
        *args,
        **kwargs
    )


def LOGI(*args, **kwargs):
    print(
        time.strftime("[I] "),
        *args,
        **kwargs
    )


def LOGW(*args, **kwargs):
    print(
        time.strftime("[W] "),
        *args,
        **kwargs
    )


def LOGE(*args, **kwargs):
    print(
        time.strftime(f"[E] "),
        *args,
        **kwargs
    )
