# Reference site

GitHub Pages serves the checked-in files in `site/`: no Jekyll or deployment-time
build. Firmware and evidence are not uploaded.

The home page has a brief overview and direct release/firmware downloads. The shared
desktop sidebar/mobile menu is the firmware directory; each family has a
control page. Release and development-only entries stay separate. Share a
firmware page URL for direct access.

A small `navigation.js` loads `reference.json` once and replaces only the main content
on navigation. The sidebar, mobile menu and footer stay as ordinary HTML.
Back/Forward and direct links work; if JavaScript or the content bundle is
unavailable, links load the static pages normally. There is no framework,
service worker or Node toolchain. The generated bundle avoids fetching another
page on each click; it is not a second source of content.

Edit `docs/firmware_reference.md` for controls or `README.md`'s Quick start for
installation. The home page renders that exact section, not a separate copy.
Refresh the HTML using GitHub's Markdown
renderer (requires authenticated `gh` and network):

```sh
make reference-html
make test
```

The color table drives navigation, so there is no second firmware catalog to edit.
Versions on the same family page share one color, matching `menu_colors` in
`firmware/images.hpp`; different families must use distinct colors.
The renderer uses GitHub Markdown and writes the HTML fallbacks and
SPA data from identical content; shared styling is in `site/style.css`. Regenerate
after changing the reference, renderer or `navigation.js` (its URL is content-versioned).
Commit source and generated files together.
Offline tests check reference/Quick start fingerprints, links, versions, colors
and equality between SPA content and fallbacks. The Pages workflow refuses stale pages.

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

It checks no-reload switching, the panel drawing, visible details, Back/Forward, mobile layouts and
ordinary page navigation with JavaScript disabled or SPA data missing/stale.

Repository **Settings → Pages → Source** must be **GitHub Actions**.
Pushes to `main` deploy automatically; the workflow can also be run manually.
The public URL is https://aurora-switch.ryanjarv.sh/.
