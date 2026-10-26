"""Check the production move helpers with pinned FatFs on a disposable FAT image."""

from pathlib import Path
import shutil
import subprocess
import tempfile

from setup_dependencies import DAISY, git

ROOT = Path(__file__).resolve().parents[1]


def check():
    sdk = ROOT / ".deps/Aurora-SDK/libs/libDaisy"
    if git(sdk, "rev-parse", "HEAD") != DAISY or git(
        sdk, "status", "--porcelain", "--untracked-files=no"
    ):
        raise ValueError("pinned clean libDaisy required; run make setup")
    source = sdk / "Middlewares/Third_Party/FatFs/src"
    with tempfile.TemporaryDirectory(prefix="aurora-fatfs-") as temporary:
        work = Path(temporary)
        # FatFs' embedded integer.h assumes 32-bit long. On a 64-bit host,
        # preserve the ARM widths without changing the filesystem algorithm.
        for name in ("ff.c", "ff.h", "diskio.h", "integer.h"):
            shutil.copyfile(source / name, work / name)
        integer = work / "integer.h"
        integer.write_text(
            integer.read_text()
            .replace("unsigned long\tDWORD", "unsigned int\tDWORD")
            .replace("long\t\t\t\tLONG", "int\t\t\t\tLONG")
        )
        includes = [
            "-I" + str(work),
            "-I" + str(sdk / "src/sys"),
            "-I" + str(sdk / "src"),
        ]
        objects = []
        for name, path in (
            ("fatfs", work / "ff.c"),
            ("unicode", source / "option/unicode.c"),
        ):
            obj = work / (name + ".o")
            subprocess.run(
                ["cc", "-std=c99", *includes, "-c", str(path), "-o", str(obj)],
                check=True,
            )
            objects.append(str(obj))
        executable = work / "check-install"
        subprocess.run(
            [
                "c++",
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                *includes,
                str(ROOT / "firmware/fatfs_install_test.cpp"),
                *objects,
                "-o",
                str(executable),
            ],
            check=True,
        )
        image = work / "test.fat"
        with image.open("wb") as stream:
            stream.truncate(64 * 1024 * 1024)
        subprocess.run(["mkfs.fat", "-F", "32", str(image)], check=True)
        subprocess.run([str(executable), str(image)], check=True)
        print("PASS: real FatFs move, remount/readback and restoration on disposable FAT32")


if __name__ == "__main__":
    check()
