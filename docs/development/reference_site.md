# Reference site

GitHub Pages serves the checked-in files in `site/`: no Jekyll or deployment-time
build. Firmware and evidence are not uploaded.

The index has a card for each version/color. Each firmware family gets a control
view, with a desktop sidebar, a native mobile menu and links to its control groups.
Older index fragments still find the matching card; share a firmware page URL for
direct access. Release and development-only entries stay separate.

Preact switches views without reloading the page. `app.js` loads the generated
`reference.json` once and uses browser history for Back/Forward. HTML fallbacks
keep every direct URL working on GitHub Pages, even without JavaScript or if
SPA startup fails. No service worker, routing package or Node toolchain is needed.
The pinned framework and its upstream notice are in `site/vendor/`.

Edit `docs/firmware_reference.md`, then refresh the HTML using GitHub's Markdown
renderer (requires authenticated `gh` and network):

```sh
make reference-html
make test
```

The color table drives navigation, so there is no second firmware catalog to edit.
The renderer uses one GitHub Markdown request and writes the HTML fallbacks and
SPA data from identical content; shared styling is in `site/style.css`. Regenerate
after changing the reference, renderer or `app.js` (its URL is content-versioned).
Commit source and generated files together.
Offline tests check fingerprints, links, versions, colors, pinned Preact bytes
and equality between SPA content and fallbacks. The Pages workflow refuses stale pages.

`site/aurora-panel.svg` is an original simplified drawing of the manual's panel
layout, not copied artwork. Knobs 1–6 and buttons 7–9 match table labels added by
the renderer; functions still come only from Markdown. Secondary LED/settings
details and source caveats live in one collapsed section per firmware.

For changes to navigation, also run the real-browser smoke test against an
installed Firefox and a running `geckodriver` (no Python packages required):

```sh
geckodriver --port 4444
# In another terminal; use --firefox /path/to/firefox if necessary:
python3 scripts/check_reference_browser.py --webdriver http://127.0.0.1:4444
```

It checks no-reload switching, the panel drawing, expandable details, Back/Forward, mobile layouts and
ordinary page navigation with JavaScript disabled or SPA data missing/stale.

Repository **Settings → Pages → Source** must be **GitHub Actions**.
Pushes to `main` deploy automatically; the workflow can also be run manually.
The public URL is https://aurora-switch.ryanjarv.sh/.
