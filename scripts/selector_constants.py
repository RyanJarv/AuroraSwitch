"""Read the shared literal definitions; do not maintain a second numeric map."""
from pathlib import Path
import re


def read_constants(header: str) -> dict[str, int]:
    definitions = re.findall(r"^#define SELECTOR_(\w+) (0x[0-9a-fA-F]+|[0-9]+)$", header, re.M)
    names = [name for name, _ in definitions]
    if (not definitions or len(set(names)) != len(names)
            or len(definitions) != len(re.findall(r"^#define SELECTOR_", header, re.M))):
        raise ValueError("selector constants must have unique literal integer definitions")
    return {name: int(value, 16 if value.startswith("0x") else 10)
            for name, value in definitions}


CONSTANTS = read_constants((Path(__file__).resolve().parents[1]
                           / "support/selector_constants.h").read_text())
