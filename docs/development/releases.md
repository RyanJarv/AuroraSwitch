# Builds and releases

Tag pushes run checks and an isolated USB build with the pinned Arm toolchain.
The workflow attaches BIN, ELF, MAP, manifest, and checksums to an existing
release, preserving its notes and status. If none exists, it creates a
**draft prerelease**. Existing assets are never overwritten; payload firmware is excluded.

The build and artifact upload passed in [CI](https://github.com/RyanJarv/AuroraSwitch/actions/runs/37555710183).
The quickstart uses `make download-release` to download the latest published release, check its
manifest hashes, tag/source identity, and real-USB configuration, then reuse the
existing payload authentication and copying code. These are consistency checks,
not signatures or proof of physical reliability.
Use `RELEASE_TAG=v0.1.0` to select a specific release instead.

```sh
git tag -a v0.1.0-beta.1 -m "AuroraSwitch beta test build"
git push origin v0.1.0-beta.1
```

Keep drafts unpublished until the [recovery checks](recovery_release_gate.md)
pass. Do not move a release tag.

For a build-only run, without a tag or release:

```sh
gh workflow run firmware-release.yml --ref main
```

Download the run's firmware artifact and check it with
`sha256sum --check SHA256SUMS`. Install only `AuroraSwitch.bin`; keep the other
files for reports. Successful CI is a software check, not hardware qualification.
