#!/usr/bin/env python3
"""
libclang Deployment Script
===========================

Downloads and deploys libclang for B6x MCP Server.

Usage:
    python scripts/deploy_libclang.py           # Download and deploy
    python scripts/deploy_libclang.py --check   # Check current status
    python scripts/deploy_libclang.py --local "C:/path/to/libclang.dll"  # Copy from local

Author: B6x MCP Server Team
"""

import os
import sys
import platform
import shutil
import zipfile
import argparse
import subprocess
from pathlib import Path
from urllib.request import urlretrieve
from tempfile import TemporaryDirectory


# ============================================================================
# Configuration
# ============================================================================

PROJECT_ROOT = Path(__file__).parent.parent
LIB_DIR = PROJECT_ROOT / "lib" / "clang"

# LLVM download URLs (LLVM 18.1.8 - latest stable as of 2024)
LLVM_VERSION = "18.1.8"
LLVM_DOWNLOADS = {
    "Windows": {
        "url": f"https://github.com/llvm/llvm-project/releases/download/llvmorg-{LLVM_VERSION}/LLVM-{LLVM_VERSION}-win64.exe",
        "file": f"LLVM-{LLVM_VERSION}-win64.exe",
        "lib_name": "libclang.dll",
    },
    "Linux": {
        "url": f"https://github.com/llvm/llvm-project/releases/download/llvmorg-{LLVM_VERSION}/clang+llvm-{LLVM_VERSION}-x86_64-linux-gnu.tar.xz",
        "file": f"clang+llvm-{LLVM_VERSION}-x86_64-linux-gnu.tar.xz",
        "lib_name": "libclang.so.18.1",
    },
    "Darwin": {
        "url": f"https://github.com/llvm/llvm-project/releases/download/llvmorg-{LLVM_VERSION}/LLVM-{LLVM_VERSION}-arm64-macOS.tar.gz",
        "file": f"LLVM-{LLVM_VERSION}-arm64-macOS.tar.gz",
        "lib_name": "libclang.dylib",
    },
}

# Minimum required version
MIN_VERSION = (16, 0, 0)


# ============================================================================
# Platform Detection
# ============================================================================

def get_platform_dir() -> str:
    """Get platform-specific directory name"""
    system = platform.system()
    machine = platform.machine().lower()

    if system == "Windows":
        return "win-x64"
    elif system == "Darwin":
        return "darwin-arm64" if machine in ["arm64", "aarch64"] else "darwin-x64"
    else:  # Linux
        return "linux-x64"


def get_libclang_name() -> str:
    """Get platform-specific library name"""
    system = platform.system()
    if system == "Windows":
        return "libclang.dll"
    elif system == "Darwin":
        return "libclang.dylib"
    else:
        # Linux - try multiple versions
        return "libclang.so.18"


# ============================================================================
# Status Check
# ============================================================================

def check_deployment() -> dict:
    """Check current libclang deployment status"""
    platform_dir = get_platform_dir()
    libclang_name = get_libclang_name()
    target_path = LIB_DIR / platform_dir / libclang_name

    result = {
        "platform": platform_dir,
        "target_path": str(target_path),
        "exists": target_path.exists(),
        "valid": False,
        "version": None,
    }

    if target_path.exists():
        result["size_mb"] = target_path.stat().st_size / (1024 * 1024)
        result["valid"] = result["size_mb"] > 1  # Should be at least 1MB

    # Check system LLVM
    system_paths = []
    if platform.system() == "Windows":
        system_paths = [
            Path("C:/Program Files/LLVM/bin/libclang.dll"),
            Path("C:/LLVM/bin/libclang.dll"),
        ]
    elif platform.system() == "Darwin":
        system_paths = [
            Path("/usr/local/opt/llvm/lib/libclang.dylib"),
            Path("/opt/homebrew/opt/llvm/lib/libclang.dylib"),
        ]
    else:
        system_paths = [
            Path("/usr/lib/llvm-18/lib/libclang.so.1"),
            Path("/usr/lib/llvm-17/lib/libclang.so.1"),
            Path("/usr/lib/llvm-16/lib/libclang.so.1"),
            Path("/usr/lib/libclang.so.1"),
        ]

    for sys_path in system_paths:
        if sys_path.exists():
            result["system_libclang"] = str(sys_path)
            break

    return result


def print_status():
    """Print deployment status"""
    status = check_deployment()

    print("=" * 60)
    print("libclang Deployment Status")
    print("=" * 60)
    print(f"Platform: {status['platform']}")
    print(f"Target path: {status['target_path']}")
    print(f"Deployed: {'[OK] Yes' if status['exists'] else '[X] No'}")

    if status["exists"]:
        print(f"Size: {status.get('size_mb', 0):.2f} MB")
        print(f"Valid: {'[OK]' if status['valid'] else '[X]'}")

    if "system_libclang" in status:
        print(f"System libclang: {status['system_libclang']}")
    else:
        print("System libclang: [X] Not found")

    print("=" * 60)

    return status["exists"] and status["valid"]


# ============================================================================
# Deployment Methods
# ============================================================================

def deploy_from_local(local_path: str) -> bool:
    """Deploy libclang from a local file"""
    source = Path(local_path)
    if not source.exists():
        print(f"[X] Error: File not found: {local_path}")
        return False

    platform_dir = get_platform_dir()
    target_dir = LIB_DIR / platform_dir
    target_dir.mkdir(parents=True, exist_ok=True)

    # Determine target filename
    if platform.system() == "Windows":
        target_name = "libclang.dll"
    elif platform.system() == "Darwin":
        target_name = "libclang.dylib"
    else:
        target_name = source.name  # Keep original name on Linux

    target = target_dir / target_name

    try:
        shutil.copy2(source, target)
        print(f"[OK] Deployed: {target}")
        print(f"   Size: {target.stat().st_size / (1024*1024):.2f} MB")
        return True
    except Exception as e:
        print(f"[X] Error copying file: {e}")
        return False


def deploy_from_llvm_install() -> bool:
    """Deploy libclang from LLVM installation"""
    status = check_deployment()

    if "system_libclang" not in status:
        print("[X] No LLVM installation found")
        print("\nPlease install LLVM from: https://github.com/llvm/llvm-project/releases")
        return False

    return deploy_from_local(status["system_libclang"])


def download_and_deploy() -> bool:
    """Download libclang and deploy (Windows only - full installer)"""
    system = platform.system()

    if system != "Windows":
        print("[X] Automatic download only supported on Windows")
        print(f"\nFor {system}, please:")
        print("1. Install LLVM from: https://github.com/llvm/llvm-project/releases")
        print("2. Run: python scripts/deploy_libclang.py --from-install")
        return False

    config = LLVM_DOWNLOADS.get(system)
    if not config:
        print(f"[X] Unsupported platform: {system}")
        return False

    print(f"[...] Downloading LLVM {LLVM_VERSION}...")
    print(f"   URL: {config['url']}")

    with TemporaryDirectory() as tmpdir:
        download_path = Path(tmpdir) / config["file"]

        try:
            # Download
            def progress_hook(count, block_size, total_size):
                percent = min(100, int(count * block_size * 100 / total_size))
                sys.stdout.write(f"\r   Progress: {percent}%")
                sys.stdout.flush()

            urlretrieve(config["url"], download_path, progress_hook)
            print()  # New line after progress

            # For Windows, we need to extract from the installer
            # The installer is a 7-zip self-extracting archive
            print("[...] Extracting libclang.dll...")

            # Try 7z first
            result = subprocess.run(
                ["7z", "e", "-y", f"-o{tmpdir}/extracted", str(download_path), "bin/libclang.dll"],
                capture_output=True,
                text=True
            )

            if result.returncode != 0:
                # Try with 7za
                result = subprocess.run(
                    ["7za", "e", "-y", f"-o{tmpdir}/extracted", str(download_path), "bin/libclang.dll"],
                    capture_output=True,
                    text=True
                )

            if result.returncode != 0:
                print("[X] Failed to extract from installer")
                print("   Please install 7-Zip: https://www.7-zip.org/download.html")
                print(f"   Or manually install LLVM from: {download_path}")
                return False

            # Find extracted file
            extracted = Path(tmpdir) / "extracted" / "libclang.dll"
            if not extracted.exists():
                # Try alternate location
                extracted = Path(tmpdir) / "extracted" / "bin" / "libclang.dll"

            if not extracted.exists():
                print("[X] libclang.dll not found in extracted files")
                return False

            # Deploy
            return deploy_from_local(str(extracted))

        except Exception as e:
            print(f"[X] Download failed: {e}")
            return False


def deploy_minimal() -> bool:
    """
    Deploy minimal libclang by extracting only essential files.
    This is the recommended approach for B6x MCP Server.
    """
    print("=" * 60)
    print("Minimal libclang Deployment")
    print("=" * 60)

    # Check if we can use system libclang first
    status = check_deployment()
    if "system_libclang" in status:
        print(f"\n[OK] Found system libclang: {status['system_libclang']}")
        print("You can deploy from system installation with:")
        print("  python scripts/deploy_libclang.py --from_install")
        return True

    system = platform.system()

    # Platform-specific instructions
    if system == "Windows":
        print("\n[*] Windows Deployment Options:")
        print("-" * 40)
        print("Option 1: Install LLVM (Recommended)")
        print("  1. Download from: https://github.com/llvm/llvm-project/releases")
        print(f"     Recommended: LLVM-{LLVM_VERSION}-win64.exe")
        print("  2. Run installer")
        print("  3. Run: python scripts/deploy_libclang.py --from_install")
        print()
        print("Option 2: Download libclang.dll only")
        print("  1. Visit: https://github.com/llvm/llvm-project/releases")
        print("  2. Download the .exe installer")
        print("  3. Extract with 7-Zip: bin/libclang.dll")
        print(f"  4. Copy to: {LIB_DIR / 'win-x64' / 'libclang.dll'}")

    elif system == "Darwin":
        print("\n[*] macOS Deployment Options:")
        print("-" * 40)
        print("Option 1: Homebrew (Recommended)")
        print("  brew install llvm")
        print("  python scripts/deploy_libclang.py --from_install")
        print()
        print("Option 2: Download from GitHub")
        print("  1. Download from: https://github.com/llvm/llvm-project/releases")
        print(f"  2. Extract and copy libclang.dylib to: {LIB_DIR / 'darwin-*'}")

    else:  # Linux
        print("\n[*] Linux Deployment Options:")
        print("-" * 40)
        print("Option 1: Package Manager (Recommended)")
        print("  Ubuntu/Debian: sudo apt install libclang-dev")
        print("  Fedora: sudo dnf install clang-devel")
        print("  Arch: sudo pacman -S clang")
        print("  Then: python scripts/deploy_libclang.py --from_install")
        print()
        print("Option 2: Download from GitHub")
        print("  1. Download from: https://github.com/llvm/llvm-project/releases")
        print("  2. Extract and copy libclang.so to lib/clang/linux-x64/")

    print()
    print("=" * 60)
    print("After deployment, verify with:")
    print("  python scripts/deploy_libclang.py --check")
    print("=" * 60)

    return False


# ============================================================================
# Main
# ============================================================================

def main():
    parser = argparse.ArgumentParser(description="Deploy libclang for B6x MCP Server")
    parser.add_argument("--check", action="store_true", help="Check deployment status")
    parser.add_argument("--from_install", action="store_true", help="Deploy from LLVM installation")
    parser.add_argument("--local", type=str, help="Deploy from local file path")
    parser.add_argument("--download", action="store_true", help="Download and deploy (Windows only)")

    args = parser.parse_args()

    # Ensure directory exists
    platform_dir = get_platform_dir()
    target_dir = LIB_DIR / platform_dir
    target_dir.mkdir(parents=True, exist_ok=True)

    if args.check:
        success = print_status()
        sys.exit(0 if success else 1)

    if args.local:
        success = deploy_from_local(args.local)
        sys.exit(0 if success else 1)

    if args.from_install:
        success = deploy_from_llvm_install()
        if success:
            print_status()
        sys.exit(0 if success else 1)

    if args.download:
        success = download_and_deploy()
        if success:
            print_status()
        sys.exit(0 if success else 1)

    # Default: show deployment guide
    deploy_minimal()


if __name__ == "__main__":
    main()
