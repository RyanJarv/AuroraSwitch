# Builds and releases

Tag pushes run checks and an isolated USB build with the pinned Arm toolchain.
The workflow creates a **draft prerelease** containing the BIN, ELF, MAP,
manifest, and checksums. It does not include payload firmware or overwrite releases.

The build and artifact upload passed in [CI](https://github.com/RyanJarv/AuroraSwitch/actions/runs/37555710183).
The tag-triggered release job has not run yet. Once a release is published,
switch the README quickstart to downloading its BIN instead of requiring the Arm toolchain.

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
