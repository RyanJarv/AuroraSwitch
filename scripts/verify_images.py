"""Compile the existing byte/vector authenticator and check user-supplied images."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def verify(directory: Path) -> None:
    tls = ROOT / ".deps/mbedtls"
    with tempfile.TemporaryDirectory(prefix="aurora-image-check-") as temporary:
        destination = Path(temporary)
        objects = []
        for name in ("sha256", "platform_util"):
            obj = destination / (name + ".o")
            subprocess.run(["cc", "-std=c99", '-DMBEDTLS_CONFIG_FILE="sha256_config.h"',
                "-I" + str(ROOT / "firmware"), "-I" + str(tls / "include"), "-c",
                str(tls / "library" / (name + ".c")), "-o", str(obj)], check=True)
            objects.append(str(obj))
        executable = destination / "authenticate-image"
        subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
            '-DMBEDTLS_CONFIG_FILE="sha256_config.h"', "-I" + str(tls / "include"),
            "-I" + str(ROOT / "firmware"), str(ROOT / "firmware/authenticate_image.cpp"),
            *objects, "-o", str(executable)], check=True)
        for kind, filename in (("fdn", "AR_FDN_v1_2_2.bin"), ("spectral", "Aurora_v1_4_4.bin")):
            subprocess.run([str(executable), kind, str(directory / filename)], check=True)


if __name__ == "__main__":
    if len(sys.argv) != 2 or not sys.argv[1]:
        raise SystemExit("usage: make verify-images FIRMWARE_DIR=/path/to/user-supplied/files")
    verify(Path(sys.argv[1]).resolve())
