# ======================== LEGAL DISCLAIMER (CORE COMPLIANCE) ========================
DISCLAIMER = """
===========================================================================
IMPORTANT LEGAL DISCLAIMER
===========================================================================
1. Copyright Ownership: Live2D Cubism Core library is the exclusive property of Live2D Inc.
   All intellectual property rights (including copyright) belong to Live2D Inc.
   
2. Distribution Restriction: Under Live2D's official Terms of Service, third parties are 
   prohibited from distributing/sharing Cubism Core files. This script only provides an 
   "auto-guided download" function and does NOT store or forward any Core files.
   
3. Compliance Requirement: You must adhere to the Live2D Cubism SDK End User License 
   Agreement (EULA). Core library usage is restricted to legal personal/commercial projects 
   only—reverse engineering or secondary distribution of Core files is strictly forbidden.
   
4. Liability Waiver: This script is provided as a convenience tool only. It bears no 
   responsibility for the integrity or compatibility of Core files. Any compliance issues 
   arising from the use of this script are the sole responsibility of the user.

OFFICIAL ACQUISITION CHANNELS (RECOMMENDED):
   - Cubism Official Website: https://www.live2d.com/sdk/about/
   - Developer Documentation: https://docs.live2d.com/en/cubism-sdk-manual/top/
   - License Agreement: https://www.live2d.com/eula/live2d-software-license-agreement/
===========================================================================
"""

import os
import platform
import re
import subprocess
import sys

from setuptools import setup, find_packages, Extension, Command
from setuptools.command.build_ext import build_ext
from setuptools.command.install import install
from setuptools.command.bdist_wheel import bdist_wheel


# Read version from package (single source of truth)
with open(os.path.join(os.path.dirname(__file__), "package", "live2d", "__init__.py"), encoding="utf-8") as _f:
    for _line in _f:
        if _line.startswith("__version__"):
            VERSION = _line.split('"')[1]
            break
CUBISM_SDK_DISTRIBUTION = (
    "https://cubism.live2d.com/sdk-native/bin/CubismSdkForNative-5-r.4.1.zip"
)

NAME = "live2d-py"
DESCRIPTION = "Live2D Python SDK"
LONG_DESCRIPTION = (
    open("README.md", "r", encoding="utf-8")
    .read()
    .replace(
        "./", "https://raw.githubusercontent.com/Arkueid/live2d-py/refs/heads/main/"
    )
)
AUTHOR = "Arkueid"
AUTHOR_EMAIL = "thetardis@qq.com"
URL = "https://github.com/Arkueid/live2d-py"
REQUIRES_PYTHON = ">=3.2"
INSTALL_REQUIRES = ["numpy", "pyopengl", "pillow"]


"""
===============Live2D Cubism Core Auto-Download Script============================
Purpose: Compliantly acquire the Live2D Cubism Core library (distribution prohibited by Live2D's official terms)
"""

import os
import sys
import urllib.request
import zipfile
import shutil

# ======================== CONFIGURATION (ADJUST FOR YOUR PROJECT) ========================
# NOTE: Obtain the latest official download links from Live2D's website!

# Temporary download path
TEMP_ZIP_PATH = os.path.join(os.path.dirname(__file__), "cubism_sdk_temp.zip")
EXTRACT_DIR = os.path.join(os.path.dirname(__file__), "csmsdk_temp")
DST_DIR = os.path.join(os.path.dirname(__file__), "Live2D", "V3")


def print_disclaimer():
    """Print legal disclaimer and confirm user acknowledgment"""
    print(DISCLAIMER)


def download_sdk(url, save_path):
    """Download Core library zip file"""
    print(f"\nStarting Core library download from official server: {url}")
    print("   (Download speed depends on your network—do not interrupt the process)")
    try:
        import ssl
        ctx = ssl.create_default_context()
        req = urllib.request.Request(url, headers={
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36"
        })
        with urllib.request.urlopen(req, context=ctx, timeout=60) as resp:
            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0
            with open(save_path, "wb") as f:
                while True:
                    chunk = resp.read(65536)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    if total > 0:
                        percent = min(100.0, downloaded * 100.0 / total)
                        sys.stdout.write(
                            f"\r   Progress: {percent:.1f}% "
                            f"({downloaded / 1024 / 1024:.2f}MB/{total / 1024 / 1024:.2f}MB)"
                        )
                        sys.stdout.flush()
            print()
    except Exception as e:
        raise RuntimeError(
            f"Download failed: {str(e)}\n"
            f"Check your network or download manually from: https://www.live2d.com/download/cubism-sdk/"
        )


def print_progress(block_num, block_size, total_size):
    """Print download progress bar"""
    if total_size == 0:
        return
    downloaded = block_num * block_size
    percent = min(100.0, downloaded * 100.0 / total_size)
    sys.stdout.write(
        f"\r   Progress: {percent:.1f}% ({downloaded / 1024 / 1024:.2f}MB/{total_size / 1024 / 1024:.2f}MB)"
    )
    sys.stdout.flush()


def extract_all(zip_path, output_dir):
    """Extract Core library from zip archive"""
    print(f"\nExtracting Core library to: {output_dir}")
    os.makedirs(output_dir, exist_ok=True)
    try:
        with zipfile.ZipFile(zip_path, "r") as zip_ref:
            zip_ref.extractall(output_dir)
        print("Extraction completed successfully")
    except Exception as e:
        raise RuntimeError(f"Extraction failed: {str(e)}")


def setup_directory():
    print("[download_csmsdk] Download start.")

    core_found = False
    framework_found = False

    core_path = None
    framework_path = None
    for i in os.walk(EXTRACT_DIR):
        dir_path = i[0]
        dir_name = os.path.split(dir_path)[-1]
        if dir_name == "Core":
            core_found = True
            core_path = dir_path
        elif dir_name == "Framework":
            framework_found = True
            framework_path = dir_path
        if core_found and framework_found:
            break

    if not core_path or not framework_path:
        print("[download_csmsdk] Setup directory start.")
    else:
        try:
            dst_core_path = os.path.join(DST_DIR, "Core")
            if os.path.exists(dst_core_path):
                shutil.rmtree(dst_core_path)
            shutil.move(core_path, DST_DIR)
            dst_framework_path = os.path.join(DST_DIR, "Framework")
            if os.path.exists(dst_framework_path):
                shutil.rmtree(dst_framework_path)
            shutil.move(framework_path, DST_DIR)
            print("[download_csmsdk] Setup directory end.")
        except Exception as e:
            print("[download_csmsdk] Setup build directory failed with error:", e)


def clean_temp_files(temp_path):
    """Clean up temporary files"""
    os.remove(temp_path)


def execute_download(sdk_url: str) -> bool:

    print("[download_csmsdk] Download start.")
    print_disclaimer()
    success = False
    if not os.path.exists(TEMP_ZIP_PATH):
        try:
            download_sdk(sdk_url, TEMP_ZIP_PATH)
            print("[download_csmsdk] Download success.")
            success = True
        except Exception as e:
            print("[download_csmsdk] Error downloading", e)
            clean_temp_files(TEMP_ZIP_PATH)
            success = False
    else:
        print("[download_csmsdk] Already downloaded.")
    try:
        extract_all(TEMP_ZIP_PATH, EXTRACT_DIR)
        setup_directory()
        success = True
    except Exception as e:
        print("[download_csmsdk] Error extracting library.", e)
        success = False
    print("[download_csmsdk] Download end.")
    return success


"""
============================ END OF Live2D SDK DOWNLOAD SCRIPT =============================
"""


def is_virtualenv():
    return "VIRTUAL_ENV" in os.environ


def get_base_python_path(venv_path):
    return re.search(
        "home = (.*)\n", open(os.path.join(venv_path, "pyvenv.cfg"), "r").read()
    ).group(1)


cmake_built = False


CORE_LIB_PATHS = [
    os.path.join(os.path.dirname(__file__), "Live2D", "Core"),
    os.path.join(os.path.dirname(__file__), "Live2D", "V3", "Core"),
]

def is_sdk_present():
    """Check if Cubism SDK is already present (downloaded or vendored)."""
    for p in CORE_LIB_PATHS:
        if os.path.isdir(p) and os.listdir(p):
            return True
    return False

def run_cmake():
    global cmake_built
    if cmake_built:
        return

    if not is_sdk_present():
        if not execute_download(CUBISM_SDK_DISTRIBUTION):
            raise RuntimeError("Download failed. Please download the Cubism SDK "
                               "manually from https://www.live2d.com/sdk/download/native/ "
                               "and extract to Live2D/")
    else:
        print("[cmake] Cubism SDK already present, skipping download.")

    cmake_args = ["-DBUILD_V2CPP=ON"]
    build_args = ["--config", "Release", "--target", "Live2DV2Wrapper", "--target", "Live2DWrapper"]

    if platform.system() == "Windows":
        if platform.python_compiler().find("64 bit") > 0:
            print("Building for 64 bit")
            cmake_args += ["-A", "x64"]
        else:
            print("Building for 32 bit")
            cmake_args += ["-A", "Win32"]
        # native options
        build_args += ["--", "/m:2"]
    else:
        cmake_args += ["-DCMAKE_BUILD_TYPE=" + "Release"]
        build_args += ["--", "-j2"]
    build_folder = os.path.join(os.getcwd(), "build")

    if not os.path.exists(build_folder):
        os.makedirs(build_folder)

    if is_virtualenv():
        python_installation_path = get_base_python_path(os.environ["VIRTUAL_ENV"])
    else:
        python_installation_path = os.path.split(sys.executable)[0]
    print("Python installation path: " + python_installation_path)
    sys.stdout.flush()

    cmake_args += ["-DPYTHON_INSTALLATION_PATH=" + python_installation_path]
    cmake_args += ["-UVIEWER"]

    cmake_setup = ["cmake", ".."] + cmake_args
    cmake_build = ["cmake", "--build", "."] + build_args

    print("Building extension for Python {}".format(sys.version.split("\n", 1)[0]))
    print("Invoking CMake setup: '{}'".format(" ".join(cmake_setup)))
    sys.stdout.flush()
    subprocess.check_call(cmake_setup, cwd=build_folder)
    print("Invoking CMake build: '{}'".format(" ".join(cmake_build)))
    sys.stdout.flush()
    subprocess.check_call(cmake_build, cwd=build_folder)

    cmake_built = True


class FakeExtension(Extension):

    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[], py_limited_api=True)
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):

    def run(self):
        run_cmake()


class BuildWheel(bdist_wheel):
    def run(self):
        run_cmake()
        bdist_wheel.run(self)


class Install(install):
    def run(self):
        run_cmake()
        install.run(self)


class Download(Command):
    """Download Live2D SDK"""

    description = "Download Live2D SDK"

    def initialize_options(self):
        pass

    def finalize_options(self):
        pass

    def run(self):
        execute_download(CUBISM_SDK_DISTRIBUTION)
        print("Download completed successfully")
        

setup(
    name=NAME,
    version=VERSION,
    description=DESCRIPTION,
    long_description=LONG_DESCRIPTION,
    long_description_content_type="text/markdown",
    author=AUTHOR,
    author_email=AUTHOR_EMAIL,
    license="LICENSE",
    url=URL,
    install_requires=INSTALL_REQUIRES,
    ext_modules=[FakeExtension("LAppModelWrapper", ".")],
    cmdclass={"build_ext": CMakeBuild, "bdist_wheel": BuildWheel, "install": Install, "download": Download},
    packages=find_packages(where="package"),
    package_data={"": ["**/*.pyd", "**/*.so", "**/*.pyi", "**/*.py"]},
    package_dir={"": "package"},
    keywords=["Live2D", "Cubism Live2D", "Cubism SDK", "Cubism SDK for Python"],
    python_requires=REQUIRES_PYTHON,
)
