# AuroraSwitch contributor instructions

Keep this a small Aurora firmware selector. Reuse existing helpers; avoid new
frameworks, optional modes, and features without a concrete need. Use `gh` for
GitHub operations. Preserve unrelated local changes.

## Website and UI

- Make the interface understandable at a glance through layout, grouping,
  labels, and visible state. Fix confusing UI instead of adding explanations.
- Label categories, firmware, colors, controls, and actions clearly. Labels
  are useful; prose teaching people how website elements work is not.
- Do not add sentences such as “click this button to change the view,” legends
  explaining website widgets, or a “how to use this website” section.
- Hardware instructions are different: show the actual module actions briefly,
  such as **Shift — next category**, **Reverse — next firmware**, and
  **Freeze — verify & launch**. Keep install/recovery steps easy to find.
- Match the firmware's category order and retain its existing firmware colors.
  Use names as well as color; similar LEDs must remain distinguishable.
- Keep public text brief. Development rationale belongs in `docs/development/`,
  not user-facing prose. Do not mention sibling/internal projects on the site.
- Edit `docs/firmware_reference.md`, `scripts/render_reference.py`, and shared
  site assets; regenerate HTML and `reference.json` with `make reference-html`.
  Do not hand-edit generated pages. Run the browser smoke test for UI changes,
  including mobile, dark theme, keyboard access, and no-JavaScript fallbacks.

## Firmware and evidence

- Preserve exact-image authentication, launch-time staged-byte verification,
  DMA cleanup, reviewed memory bounds, and ordinary stock USB recovery.
- Do not distribute third-party firmware binaries. Keep download sources and
  supported identities explicit; reject unsupported or changed images.
- Separate behavior-preserving cleanup from functional fixes. Record failing
  evidence before changing a discovered safety or behavior defect.
- Firmware changes require a fresh authenticated candidate and affected virtual
  regressions. Documentation/site-only changes do not require emulator campaigns.
- Virtual passes and USB readback are not physical handoff or recovery proof.
  Keep physical reliability and stock recovery claims bounded by actual evidence.
- Hardware access, flashing, and USB changes require the user's task to authorize
  them. Copying a build to a drive does not authorize installing it on the module.

## Workflow

`make check` runs host tests and Python syntax checks. `make reference-html`
regenerates the site using authenticated `gh`. Build/package and browser commands
are in `docs/development/`.

Keep commits focused. Push only after applicable checks pass; check hosted CI.
Publishing the site does not publish or qualify a firmware release.
