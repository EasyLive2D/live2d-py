#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Live2D Cubism Core Auto-Download Script
Purpose: Compliantly acquire the Live2D Cubism Core library (distribution prohibited by Live2D's official terms)
"""

import os
import sys
import urllib.request
import zipfile
import shutil

# ======================== LEGAL DISCLAIMER (CORE COMPLIANCE) ========================
DISCLAIMER = """
===========================================================================
⚠️  IMPORTANT LEGAL DISCLAIMER
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

📌 OFFICIAL ACQUISITION CHANNELS (RECOMMENDED):
   - Cubism Official Website: https://www.live2d.com/sdk/about/
   - Developer Documentation: https://docs.live2d.com/en/cubism-sdk-manual/top/
   - License Agreement: https://www.live2d.com/eula/live2d-software-license-agreement/
===========================================================================
"""

# ======================== CONFIGURATION (ADJUST FOR YOUR PROJECT) ========================
# NOTE: Obtain the latest official download links from Live2D's website!

# Temporary download path
TEMP_ZIP_PATH = os.path.join(os.path.dirname(__file__), "cubism_sdk_temp.zip")
EXTRACT_DIR = os.path.join(os.path.dirname(__file__), "csmsdk_temp")
DST_DIR = os.path.join(os.path.dirname(__file__), "Live2D")


def print_disclaimer():
    """Print legal disclaimer and confirm user acknowledgment"""
    print(DISCLAIMER)


def download_sdk(url, save_path):
    """Download Core library zip file"""
    print(f"\n📥 Starting Core library download from official server: {url}")
    print("   (Download speed depends on your network—do not interrupt the process)")
    try:
        urllib.request.urlretrieve(
            url,
            save_path,
            reporthook=lambda block_num, block_size, total_size: print_progress(block_num, block_size, total_size)
        )
    except Exception as e:
        raise RuntimeError(
            f"❌ Download failed: {str(e)}\n"
            f"👉 Check your network or download manually from: https://www.live2d.com/download/cubism-sdk/"
        )


def print_progress(block_num, block_size, total_size):
    """Print download progress bar"""
    if total_size == 0:
        return
    downloaded = block_num * block_size
    percent = min(100.0, downloaded * 100.0 / total_size)
    sys.stdout.write(
        f"\r   Progress: {percent:.1f}% ({downloaded/1024/1024:.2f}MB/{total_size/1024/1024:.2f}MB)"
    )
    sys.stdout.flush()


def extract_all(zip_path, output_dir):
    """Extract Core library from zip archive"""
    print(f"\n📦 Extracting Core library to: {output_dir}")
    os.makedirs(output_dir, exist_ok=True)
    try:
        with zipfile.ZipFile(zip_path, "r") as zip_ref:
            zip_ref.extractall(output_dir)
        print("✅ Extraction completed successfully")
    except Exception as e:
        raise RuntimeError(f"❌ Extraction failed: {str(e)}")


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


def execute_download():
    import build_config

    print("[download_csmsdk] Download start.")
    print_disclaimer()
    if not os.path.exists(TEMP_ZIP_PATH):
        success = False
        try:
            download_sdk(build_config.CUBISM_SDK_DISTRIBUTION, TEMP_ZIP_PATH)
            print("[download_csmsdk] Download success.")
            success = True
        except Exception as e:
            print("[download_csmsdk] Error downloading", e)
            clean_temp_files(TEMP_ZIP_PATH)
            success = False
        if success:
            extract_all(TEMP_ZIP_PATH, EXTRACT_DIR)
            setup_directory()
    else:
        print("[download_csmsdk] Already downloaded.")
    print("[download_csmsdk] Download end.")
    