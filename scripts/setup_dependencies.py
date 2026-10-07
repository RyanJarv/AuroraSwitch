"""Fetch exact public source dependencies; no packages, firmware or programming."""
from pathlib import Path
import argparse
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PINS = {
    "Aurora-SDK": ("https://github.com/Qu-Bit-Electronix/Aurora-SDK.git",
                   "69b74a88b25e2fb4d722fc269bfd9395dd28edb5"),
    "mbedtls": ("https://github.com/Mbed-TLS/mbedtls.git",
                "2fc8413bfcb51354c8e679141b17b3f1a5942561"),
}
DAISY = "63fcabd38a20e14bc744499f0460e47925ea753e"


def git(path: Path, *args: str) -> str:
    return subprocess.check_output(["git", "-C", str(path), *args], text=True).strip()


def setup(verification_only: bool = False) -> None:
    """Fetch missing sources; refuse changed origins, revisions, or tracked files."""
    directory = ROOT / ".deps"
    directory.mkdir(exist_ok=True)
    for name, (url, revision) in PINS.items():
        if verification_only and name != "mbedtls":
            continue
        path = directory / name
        if not path.exists():
            subprocess.run(["git", "clone", "--no-checkout", url, str(path)], check=True)
            subprocess.run(["git", "-C", str(path), "checkout", "--detach", revision], check=True)
        if git(path, "remote", "get-url", "origin") != url:
            raise RuntimeError(f"unexpected dependency origin: {name}")
        if git(path, "rev-parse", "HEAD") != revision:
            raise RuntimeError(f"unexpected dependency revision: {name}; refusing to overwrite it")
        if git(path, "status", "--porcelain", "--untracked-files=no"):
            raise RuntimeError(f"modified dependency: {name}; refusing to overwrite it")
    if verification_only:
        print("Pinned host-verification dependency ready; no Arm toolchain needed.")
        return
    sdk = directory / "Aurora-SDK"
    subprocess.run(["git", "-C", str(sdk), "submodule", "update", "--init", "libs/libDaisy"], check=True)
    library = sdk / "libs/libDaisy"
    if git(library, "rev-parse", "HEAD") != DAISY:
        raise RuntimeError("unexpected libDaisy revision")
    if git(library, "status", "--porcelain", "--untracked-files=no"):
        raise RuntimeError("modified libDaisy dependency")
    print("Pinned dependencies ready; no firmware was downloaded or programmed.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--verification-only", action="store_true",
                        help="fetch only the host image verifier's SHA dependency")
    setup(parser.parse_args().verification_only)
