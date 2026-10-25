# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Patryk Strzelecki <patryk@strzelecki.info>
#
# PlatformIO post-build step. Produces in ./release:
#   LGRemoteCardputer-<version>.bin       app image for M5Launcher (SD card -> Install)
#   LGRemoteCardputer-<version>-full.bin  merged image (bootloader + partitions + app),
#                                         flash at offset 0x0 with esptool or M5Burner
import os
import re

Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

# Starts with "LGRemote": M5Launcher names the app partition after the first letters.
PROJECT_NAME = "LGRemoteCardputer"


def _app_version():
    for flag in env.get("BUILD_FLAGS", []):  # noqa: F821
        match = re.search(r'APP_VERSION=\\?"([^"\\]+)', flag)
        if match:
            return match.group(1)
    return "dev"


def _make_release(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    release_dir = os.path.join(project_dir, "release")
    os.makedirs(release_dir, exist_ok=True)

    version = _app_version()
    app_bin = os.path.join(build_dir, "firmware.bin")
    app_out = os.path.join(release_dir, f"{PROJECT_NAME}-{version}.bin")
    full_out = os.path.join(release_dir, f"{PROJECT_NAME}-{version}-full.bin")

    with open(app_bin, "rb") as src, open(app_out, "wb") as dst:
        dst.write(src.read())

    # FLASH_EXTRA_IMAGES holds bootloader, partition table and boot_app0 with their offsets.
    images = [(offset, env.subst(path)) for offset, path in env.get("FLASH_EXTRA_IMAGES", [])]
    images.append((env.subst("$ESP32_APP_OFFSET"), app_bin))
    image_args = " ".join(f"{offset} \"{path}\"" for offset, path in images)

    env.Execute(
        f"\"$PYTHONEXE\" \"$OBJCOPY\" --chip esp32s3 merge_bin -o \"{full_out}\" "
        f"--flash_mode dio --flash_freq 80m --flash_size 8MB {image_args}"
    )
    print(f"Release images:\n  {app_out}\n  {full_out}")


# "checkprogsize" runs on every build, so the images are (re)written even when the
# firmware itself did not change (e.g. after deleting the release folder).
env.AddPostAction("checkprogsize", _make_release)  # noqa: F821
