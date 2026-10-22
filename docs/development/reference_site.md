# Reference site

GitHub Pages serves the checked-in files in `site/`: no Jekyll or deployment-time
build. Firmware and evidence are not uploaded.

The home page groups firmware into blue Reverbs, green Delays, and amber
Synths / Other. Firmware pages share the same grouped sidebar/mobile directory.
The startup strip labels Shift, Reverse, and Freeze directly; firmware colors
stay unchanged. Family order within each group matches the selector catalog.
Installation stays visible below the categories. Unsupported-firmware notes
stay in the Markdown reference, not a separate website page.

Every firmware view is a complete static page. Ordinary links provide navigation
and browser history with or without JavaScript. There is no client-side router
or duplicate JSON content bundle.

Edit `docs/firmware_reference.md` for controls or `README.md`'s Quick start for
installation. The home page renders that exact section, not a separate copy.
Refresh the HTML using GitHub's Markdown
renderer (requires authenticated `gh` and network):

```sh
make reference-html
make test
```

The compiled catalog drives navigation, so there is no second firmware catalog to edit.
Versions on the same family page share one color, matching family metadata in
`firmware/images.hpp`; different families must use distinct colors.
Tests bind category membership/order to `firmware/images.hpp`. The directory shows
family names with adjacent color dots and labels; version numbers stay on control
pages. Website UX rules live in `AGENTS.md`: prefer clear labels and structure,
not paragraphs explaining website widgets.
The renderer uses GitHub Markdown and writes complete HTML pages; shared styling
is in `site/style.css`. Regenerate after changing the reference or renderer.
Commit source and generated files together. Offline tests check source/Quick
start fingerprints, links, versions and colors. The Pages workflow refuses
stale pages.

`site/aurora-panel.svg` is an original simplified drawing of the manual's panel
layout, not copied artwork. Knobs 1–6 and buttons 7–9 match table labels added by
the renderer; functions still come only from Markdown. Secondary LED/settings
details and source caveats remain visible below the controls.

For changes to navigation, also run the real-browser smoke test against an
installed Firefox and a running `geckodriver` (no Python packages required):

```sh
geckodriver --port 4444
# In another terminal; use --firefox /path/to/firefox if necessary:
python3 scripts/check_reference_browser.py --webdriver http://127.0.0.1:4444
```

It checks grouped navigation, startup labels, ordinary page links, the panel
drawing, Back/Forward, mobile/dark layouts, keyboard access, and operation without JavaScript.

Repository **Settings → Pages → Source** must be **GitHub Actions**.
Pushes to `main` deploy automatically; the workflow can also be run manually.
The public URL is https://aurora-switch.ryanjarv.sh/.

Navigation, family labels, palette colors and the Markdown lookup table come
from the compiled catalog in `firmware/images.hpp`. `make reference-html`
regenerates that table before rendering; control prose remains in Markdown.
The host metadata exporter requires only a native C++17 compiler, no SDK or
network setup. Page fingerprints include the exported catalog as well as the
renderer, so catalog-only changes require regenerated pages.
