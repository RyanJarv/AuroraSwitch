"""Compile the existing byte/vector authenticator and check user-supplied images."""

from pathlib import Path
from contextlib import contextmanager
import subprocess
import argparse
import tempfile
from setup_dependencies import PINS, git
from firmware_catalog import read_catalog

ROOT = Path(__file__).resolve().parents[1]


@contextmanager
def authenticator():
    """Compile the firmware's exact predicate once for a host operation."""
    tls = ROOT / ".deps/mbedtls"
    url, revision = PINS["mbedtls"]
    if (
        git(tls, "remote", "get-url", "origin") != url
        or git(tls, "rev-parse", "HEAD") != revision
        or git(tls, "status", "--porcelain", "--untracked-files=no")
    ):
        raise ValueError("unexpected SHA dependency; run make setup in a clean checkout")
    with tempfile.TemporaryDirectory(prefix="aurora-image-check-") as temporary:
        destination = Path(temporary)
        objects = []
        for name in ("sha256", "platform_util"):
            obj = destination / (name + ".o")
            subprocess.run(
                [
                    "cc",
                    "-std=c99",
                    '-DMBEDTLS_CONFIG_FILE="sha256_config.h"',
                    "-I" + str(ROOT / "firmware"),
                    "-I" + str(tls / "include"),
                    "-c",
                    str(tls / "library" / (name + ".c")),
                    "-o",
                    str(obj),
                ],
                check=True,
            )
            objects.append(str(obj))
        executable = destination / "authenticate-image"
        subprocess.run(
            [
                "c++",
                "-std=c++17",
                "-Wall",
                "-Wextra",
                "-Werror",
                '-DMBEDTLS_CONFIG_FILE="sha256_config.h"',
                "-I" + str(tls / "include"),
                "-I" + str(ROOT / "firmware"),
                str(ROOT / "firmware/authenticate_image.cpp"),
                *objects,
                "-o",
                str(executable),
            ],
            check=True,
        )
        yield executable


def verify(directory: Path) -> None:
    """Require every catalog file to pass the compiled firmware authenticator."""
    with authenticator() as executable:
        for image in read_catalog()["images"]:
            name = image["filename"]
            subprocess.run([str(executable), name, str(directory / name)], check=True)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    verify(args.directory.resolve())
