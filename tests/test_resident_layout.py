"""Source-bound layouts remain stable when the checkout changes."""

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import sealed_bundle as resident

HEADER = """#pragma once
#define SELECTOR_RESIDENT_BASE 0x90040000
#define SELECTOR_RESIDENT_ENTRY 0x90058001
#define SELECTOR_RESIDENT_METADATA 0x90058100
#define SELECTOR_RESIDENT_IMAGE 0x90059000
#define SELECTOR_RESIDENT_CAPACITY 0x78000
"""


class ResidentLayoutTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory(prefix="resident-source-layout-")
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.git("init", "--quiet")
        (self.root / "support").mkdir()
        self.revision = self.commit_header(HEADER)
        patcher = mock.patch.object(resident, "ROOT", self.root)
        patcher.start()
        self.addCleanup(patcher.stop)

    def git(self, *args):
        return subprocess.check_output(
            [
                "git",
                "-c",
                "user.name=Layout Test",
                "-c",
                "user.email=layout-test@example.invalid",
                "-c",
                "commit.gpgsign=false",
                "-C",
                str(self.root),
                *args,
            ],
            text=True,
            stderr=subprocess.PIPE,
        ).strip()

    def commit_header(self, text):
        (self.root / "support/resident_layout.h").write_text(text)
        self.git("add", "support/resident_layout.h")
        self.git("commit", "--quiet", "-m", "layout fixture")
        return self.git("rev-parse", "HEAD")

    def test_sealed_revision_wins_over_new_commits_dirty_header_and_missing_file(self):
        expected = resident.layout(self.revision)
        new_revision = self.commit_header(HEADER.replace("0x90059000", "0x9005a000"))
        self.assertEqual(resident.layout(new_revision)["IMAGE"], 0x9005A000)
        self.assertEqual(resident.layout(self.revision), expected)
        (self.root / "support/resident_layout.h").write_text("uncommitted incompatible header")
        self.assertEqual(resident.layout(self.revision), expected)
        (self.root / "support/resident_layout.h").unlink()
        self.assertEqual(resident.layout(self.revision), expected)

    def test_shared_header_migration_preserves_old_source_layout(self):
        expected = resident.layout(self.revision)
        shared = self.root / "support/selector_constants.h"
        shared.write_text(HEADER)
        self.git("rm", "support/resident_layout.h")
        self.git("add", "support/selector_constants.h")
        self.git("commit", "--quiet", "-m", "move constants")
        revision = self.git("rev-parse", "HEAD")
        shared.write_text("dirty shared header")
        self.assertEqual(resident.layout(revision), expected)
        self.assertEqual(resident.layout(self.revision), expected)

    def test_malformed_shared_header_never_falls_back_to_legacy(self):
        (self.root / "support/selector_constants.h").write_text("invalid definitions")
        self.git("add", "support/selector_constants.h")
        self.git("commit", "--quiet", "-m", "malformed shared constants")
        with self.assertRaisesRegex(ValueError, "layout definitions"):
            resident.layout(self.git("rev-parse", "HEAD"))

    def test_replace_refs_cannot_change_the_bound_layout(self):
        changed = self.commit_header(HEADER.replace("0x90059000", "0x9005a000"))
        self.git("replace", self.revision, changed)
        self.assertEqual(resident.layout(self.revision)["IMAGE"], 0x90059000)

    def test_unknown_and_malformed_revisions_reject(self):
        with self.assertRaisesRegex(ValueError, "source layout unavailable"):
            resident.layout("f" * 40)
        for revision in (None, "HEAD", "--help", "a" * 39, "a" * 40 + ":other-file"):
            with (
                self.subTest(revision=revision),
                self.assertRaisesRegex(ValueError, "source commit"),
            ):
                resident.layout(revision)

    def test_missing_duplicate_and_unsupported_definitions_reject(self):
        for text in (
            HEADER.replace("#define SELECTOR_RESIDENT_IMAGE 0x90059000\n", ""),
            HEADER + "#define SELECTOR_RESIDENT_IMAGE 0x9005a000\n",
            HEADER + "#define SELECTOR_RESIDENT_OTHER 0x1\n",
        ):
            revision = self.commit_header(text)
            with self.subTest(text=text), self.assertRaisesRegex(ValueError, "layout definitions"):
                resident.layout(revision)


if __name__ == "__main__":
    unittest.main()
