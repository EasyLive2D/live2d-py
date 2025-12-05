import os
import platform
import re
import subprocess
import sys

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from setuptools.command.install import install
from setuptools.command.bdist_wheel import bdist_wheel
import build_config
import download_csmsdk

NAME = "live2d-py"
VERSION = build_config.VERSION
DESCRIPTION = "Live2D Python SDK"
LONG_DESCRIPTION = open("README.md", "r", encoding="utf-8").read().replace("./", "https://raw.githubusercontent.com/Arkueid/live2d-py/refs/heads/main/")
AUTHOR = "Arkueid"
AUTHOR_EMAIL = "thetardis@qq.com"
URL = "https://github.com/Arkueid/live2d-py"
REQUIRES_PYTHON = ">=3.2"
INSTALL_REQUIRES = ["numpy", "pyopengl", "pillow"]


def is_virtualenv():
    return 'VIRTUAL_ENV' in os.environ


def get_base_python_path(venv_path):
    return re.search("home = (.*)\n", open(os.path.join(venv_path, "pyvenv.cfg"), 'r').read()).group(1)


cmake_built = False
def run_cmake():
    global cmake_built
    if cmake_built: return

    cmake_args = []
    build_args = ["--config", "Release", "--target", "Live2DWrapper"]

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

    print("Building extension for Python {}".format(sys.version.split('\n', 1)[0]))
    print("Invoking CMake setup: '{}'".format(' '.join(cmake_setup)))
    sys.stdout.flush()
    subprocess.check_call(cmake_setup, cwd=build_folder)
    print("Invoking CMake build: '{}'".format(' '.join(cmake_build)))
    sys.stdout.flush()
    subprocess.check_call(cmake_build, cwd=build_folder)

    cmake_built = True


class FakeExtension(Extension):

    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[], py_limited_api=True)
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):

    def run(self):
        download_csmsdk.execute_download()
        run_cmake()


class BuildWheel(bdist_wheel):
    def run(self):
        run_cmake()
        bdist_wheel.run(self)


class Install(install):
    def run(self):
        run_cmake()
        install.run(self)


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
    cmdclass={
        "build_ext": CMakeBuild,
        "bdist_wheel": BuildWheel,
        "install": Install
    },
    packages=["live2d"],
    package_data={"live2d": ["**/*.pyd", "**/*.so", "**/*.pyi", "**/*.py"]},
    package_dir={"live2d": "package/live2d"},
    keywords=["Live2D", "Cubism Live2D", "Cubism SDK", "Cubism SDK for Python"],
    python_requires=REQUIRES_PYTHON
)
