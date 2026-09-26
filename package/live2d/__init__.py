__version__ = "0.8.1"
__csm_version__ = "5-r.5"


def __print_banner():
      import sys
      _enc = (getattr(sys.stdout, "encoding", "") or "").lower()
      _utf8 = "utf" in _enc
      _L = "─" * 48 if _utf8 else "=" * 48
      _S = "★" if _utf8 else "*"
      _D = "·" if _utf8 else "-"

      print(f"\033[95m{_L}\033[0m\n"
            f"  \033[93m{_S}\033[0m \033[1m\033[96mlive2d-py\033[0m  \033[2m{_D}\033[0m  \033[1mNon-official live2d library\033[0m\n"
            f"      \033[2mversion\033[0m  \033[92m\033[1m{__version__}\033[0m\n"
            f"\033[95m{_L}\033[0m")

__print_banner()