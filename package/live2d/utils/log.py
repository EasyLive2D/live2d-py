import time

def Debug(*args, **kwargs):
    print(
        time.strftime(f"[DEBUG]"),
        *args,
        **kwargs
    )


def Info(*args, **kwargs):
    print(
        time.strftime("[INFO] "),
        *args,
        **kwargs
    )


def Error(*args, **kwargs):
    print(
        time.strftime(f"[ERROR]"),
        *args,
        **kwargs
    )
