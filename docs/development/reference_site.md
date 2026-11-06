# Reference site

GitHub Pages serves the checked-in `site/` files without a deployment-time build.
Pushes to `main` deploy through `.github/workflows/pages.yml`; local preview and
regeneration do not publish. Pages must use **GitHub Actions** as its source.
The public URL is <https://aurora-switch.ryanjarv.sh/>.

## Edit and preview

| Source | Content |
| --- | --- |
| `docs/firmware_reference.md` | Firmware controls, credits and source links |
| `docs/user_guide.md` | Setup and startup/status instructions reused by dialogs |
| `firmware/images.hpp` | Catalog, category order, family labels and colors |
| `scripts/render_reference.py` | Static page generation and navigation |
| `site/style.css`, `site/dialogs.js` | Shared presentation and native dialogs |

```sh
make reference-html
make check
make dev
```

Regeneration needs authenticated `gh`, network access and a native C++17 compiler
for the catalog exporter; it needs no SDK. Commit source and generated pages
together. Offline tests check source/catalog/guide fingerprints, links and colors;
the Pages workflow rejects stale pages.

Preview at <http://127.0.0.1:8000/> and stop with Ctrl+C. Override `PORT` or use
`HOST=0.0.0.0` for a VM browser. Preview serves only `site/` and needs no compiler,
login or network. Regenerate after CSS changes too: HTML records its content hash.

## Page structure and checks

The home page groups firmware in selector category order. Firmware pages share a
grouped sidebar/mobile directory; versions share their family's page and color.
Each page is complete HTML with ordinary links and browser history. Setup and
User Guide use native dialogs, with ordinary guide links when JavaScript is
unavailable. Unsupported-firmware notes stay in Markdown rather than adding pages.

`site/aurora-panel.svg` is an original simplified panel drawing. The renderer
numbers controls 1–9 against it; descriptions remain in Markdown. Website rules
are in `AGENTS.md`.

For presentation or navigation changes, run the [browser checks](testing.md#browser-checks),
including both dialogs, mobile/dark layouts, keyboard access and no-JavaScript
fallbacks. Firmware and generated run evidence are never site assets.
