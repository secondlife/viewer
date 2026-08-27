#!/usr/bin/env python3
"""Focused tests for exact production material artifact delivery."""

from __future__ import annotations

import os
from contextlib import contextmanager
from pathlib import Path
import shutil
import sys
import tempfile
import unittest


NEWVIEW_DIRECTORY = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(NEWVIEW_DIRECTORY))

from vulkan_material_artifact_delivery import (  # noqa: E402
    ArtifactDeliveryError,
    DESTINATION_DIRECTORY,
    PRODUCTION_ARTIFACTS,
    deliver_production_material_artifacts,
)


class CopyManifest:
    def __init__(self, destination_root: Path):
        self._destination_root = destination_root
        self._source_prefix = Path()
        self._destination_prefix = destination_root

    def dst_path_of(self, relpath: str) -> str:
        return str(self._destination_prefix / relpath)

    @contextmanager
    def prefix(self, *, src: str, dst: str):
        previous_source = self._source_prefix
        previous_destination = self._destination_prefix
        self._source_prefix = Path(src)
        self._destination_prefix = self._destination_root / dst
        try:
            yield
        finally:
            self._source_prefix = previous_source
            self._destination_prefix = previous_destination

    def path(self, src: str, dst: str | None = None) -> int:
        source = self._source_prefix / src
        output = self._destination_prefix / (dst or src)
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, output)
        return 1

    def remove(self, *paths: str) -> None:
        for raw_path in paths:
            path = Path(raw_path)
            if path.is_dir():
                shutil.rmtree(path)
            elif path.exists():
                path.unlink()


class FailingSecondCopyManifest(CopyManifest):
    def __init__(self, destination_root: Path):
        super().__init__(destination_root)
        self._copy_count = 0

    def path(self, src: str, dst: str | None = None) -> int:
        self._copy_count += 1
        if self._copy_count == 2:
            raise OSError("injected second-copy failure")
        return super().path(src, dst)


class ArtifactDeliveryTest(unittest.TestCase):
    def setUp(self) -> None:
        self._temporary_directory = tempfile.TemporaryDirectory()
        self._root = Path(self._temporary_directory.name)
        self._source = self._root / "generated"
        self._destination_root = self._root / "package"
        self._source.mkdir()
        self._manifest = CopyManifest(self._destination_root)
        self._source_bytes = {
            "material.production.vert.spv": b"vertex-production-bytes",
            "material.production.frag.spv": b"fragment-production-bytes",
        }
        for name, contents in self._source_bytes.items():
            (self._source / name).write_bytes(contents)

    def tearDown(self) -> None:
        self._temporary_directory.cleanup()

    @property
    def destination(self) -> Path:
        return self._destination_root / DESTINATION_DIRECTORY

    def test_delivers_only_exact_production_files_with_identical_bytes(self) -> None:
        (self._source / "material.production.vert.reflect.json").write_text("{}")
        (self._source / "material.vert.spv").write_bytes(b"diagnostic")
        delivered = deliver_production_material_artifacts(
            self._manifest, str(self._source)
        )

        expected_names = {destination for _, destination in PRODUCTION_ARTIFACTS}
        self.assertEqual(expected_names, {path.name for path in delivered})
        self.assertEqual(expected_names, {path.name for path in self.destination.iterdir()})
        for source_name, destination_name in PRODUCTION_ARTIFACTS:
            self.assertEqual(
                self._source_bytes[source_name],
                (self.destination / destination_name).read_bytes(),
            )

    def test_missing_source_fails_without_partial_delivery(self) -> None:
        (self._source / "material.production.frag.spv").unlink()
        self.destination.mkdir(parents=True)
        (self.destination / "stale.spv").write_bytes(b"stale")

        with self.assertRaises(ArtifactDeliveryError):
            deliver_production_material_artifacts(self._manifest, str(self._source))

        self.assertFalse(self.destination.exists())

    def test_reused_destination_is_cleaned_before_copy(self) -> None:
        self.destination.mkdir(parents=True)
        (self.destination / "stale.spv").write_bytes(b"stale")
        (self.destination / "nested").mkdir()
        (self.destination / "nested" / "evidence.json").write_text("{}")

        deliver_production_material_artifacts(self._manifest, str(self._source))

        self.assertEqual(
            {destination for _, destination in PRODUCTION_ARTIFACTS},
            {path.name for path in self.destination.iterdir()},
        )

    def test_option_off_removes_the_dedicated_destination(self) -> None:
        self.destination.mkdir(parents=True)
        (self.destination / "production.vert.spv").write_bytes(b"stale")

        delivered = deliver_production_material_artifacts(self._manifest, "")

        self.assertEqual((), delivered)
        self.assertFalse(self.destination.exists())

    def test_second_copy_failure_removes_the_partial_pair(self) -> None:
        manifest = FailingSecondCopyManifest(self._destination_root)

        with self.assertRaises(OSError):
            deliver_production_material_artifacts(manifest, str(self._source))

        self.assertFalse(self.destination.exists())

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks are unavailable")
    def test_stale_destination_symlink_is_unlinked_without_touching_target(self) -> None:
        outside = self._root / "outside"
        outside.mkdir()
        sentinel = outside / "sentinel"
        sentinel.write_bytes(b"keep")
        self.destination.parent.mkdir(parents=True)
        os.symlink(outside, self.destination, target_is_directory=True)

        deliver_production_material_artifacts(self._manifest, "")

        self.assertFalse(self.destination.exists())
        self.assertEqual(b"keep", sentinel.read_bytes())

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks are unavailable")
    def test_parent_symlink_is_rejected_without_touching_target(self) -> None:
        outside = self._root / "outside"
        outside.mkdir()
        sentinel = outside / "sentinel"
        sentinel.write_bytes(b"keep")
        vulkan_parent = self._destination_root / "app_settings" / "shaders" / "vulkan"
        vulkan_parent.parent.mkdir(parents=True)
        os.symlink(outside, vulkan_parent, target_is_directory=True)

        with self.assertRaises(ArtifactDeliveryError):
            deliver_production_material_artifacts(self._manifest, "")

        self.assertEqual(b"keep", sentinel.read_bytes())

    @unittest.skipUnless(hasattr(os, "symlink"), "symlinks are unavailable")
    def test_symlink_source_file_is_rejected(self) -> None:
        vertex = self._source / "material.production.vert.spv"
        actual = self._source / "actual.vert.spv"
        vertex.rename(actual)
        os.symlink(actual.name, vertex)

        with self.assertRaises(ArtifactDeliveryError):
            deliver_production_material_artifacts(self._manifest, str(self._source))

        self.assertFalse(self.destination.exists())


if __name__ == "__main__":
    unittest.main()
