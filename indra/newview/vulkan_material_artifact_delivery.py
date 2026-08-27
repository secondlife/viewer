"""Deliver the validated production material SPIR-V pair to app settings."""

from __future__ import annotations

from pathlib import Path
from typing import Protocol


DESTINATION_DIRECTORY = "app_settings/shaders/vulkan/legacy_normspec"
PRODUCTION_ARTIFACTS = (
    ("material.production.vert.spv", "production.vert.spv"),
    ("material.production.frag.spv", "production.frag.spv"),
)


class ArtifactDeliveryError(RuntimeError):
    """The production artifacts could not be delivered exactly."""


class Manifest(Protocol):
    def dst_path_of(self, relpath: str) -> str: ...

    def prefix(self, *, src: str, dst: str): ...

    def path(self, src: str, dst: str | None = None) -> int: ...

    def remove(self, *paths: str) -> None: ...


def _clean_destination(manifest: Manifest, destination: Path) -> None:
    # LLManifest.remove() intentionally follows normal directories. Handle a
    # stale symlink as a file so cleanup cannot escape this dedicated path.
    if destination.is_symlink():
        destination.unlink()
    else:
        manifest.remove(str(destination))


def _validate_destination_parents(manifest: Manifest) -> Path:
    destination_root = Path(manifest.dst_path_of(""))
    if destination_root.is_symlink():
        raise ArtifactDeliveryError(
            f"manifest destination root must not be a symlink: {destination_root}"
        )

    current = destination_root
    components = Path(DESTINATION_DIRECTORY).parts
    for component in components[:-1]:
        current /= component
        if current.is_symlink():
            raise ArtifactDeliveryError(
                f"artifact destination parent must not be a symlink: {current}"
            )
    return destination_root.joinpath(*components)


def deliver_production_material_artifacts(
    manifest: Manifest,
    artifact_directory: str,
) -> tuple[Path, ...]:
    """Clean the dedicated destination and optionally copy the exact pair."""

    destination = _validate_destination_parents(manifest)
    _clean_destination(manifest, destination)

    if not artifact_directory:
        return ()

    source_root = Path(artifact_directory)
    if source_root.is_symlink() or not source_root.is_dir():
        raise ArtifactDeliveryError(
            f"production artifact directory is not a real directory: {source_root}"
        )

    snapshots: list[tuple[str, str, bytes]] = []
    for source_name, destination_name in PRODUCTION_ARTIFACTS:
        source = source_root / source_name
        if source.is_symlink() or not source.is_file():
            raise ArtifactDeliveryError(
                f"production artifact is not a real file: {source}"
            )
        try:
            contents = source.read_bytes()
        except OSError as error:
            raise ArtifactDeliveryError(
                f"production artifact is unreadable: {source}: {error}"
            ) from error
        snapshots.append((source_name, destination_name, contents))

    delivered: list[Path] = []
    try:
        with manifest.prefix(src=str(source_root), dst=DESTINATION_DIRECTORY):
            for source_name, destination_name, expected_bytes in snapshots:
                if manifest.path(source_name, destination_name) != 1:
                    raise ArtifactDeliveryError(
                        f"production artifact was not copied exactly once: {source_name}"
                    )
                output = destination / destination_name
                try:
                    actual_bytes = output.read_bytes()
                except OSError as error:
                    raise ArtifactDeliveryError(
                        f"delivered artifact is unreadable: {output}: {error}"
                    ) from error
                if actual_bytes != expected_bytes:
                    raise ArtifactDeliveryError(
                        f"delivered artifact differs from validated input: {source_name}"
                    )
                delivered.append(output)

        actual_names = {path.name for path in destination.iterdir()}
        expected_names = {
            destination_name for _, destination_name in PRODUCTION_ARTIFACTS
        }
        if actual_names != expected_names:
            raise ArtifactDeliveryError(
                f"unexpected files in production artifact destination: {destination}"
            )
    except Exception:
        _clean_destination(manifest, destination)
        raise

    return tuple(delivered)
