# Reference site

GitHub Pages serves the checked-in HTML and CSS in `site/`: no Jekyll,
JavaScript or deployment-time build. Firmware and evidence are not uploaded.

The index has a card for each version/color. Each firmware family gets a control
page, with a desktop sidebar, a native mobile menu and links to its control groups.
Older index fragments still find the matching card; share a firmware page URL for
direct access. Release and development-only entries stay separate.

Edit `docs/firmware_reference.md`, then refresh the HTML using GitHub's Markdown
renderer (requires authenticated `gh` and network):

```sh
make reference-html
make test
```

The color table drives navigation, so there is no second firmware catalog to edit.
The renderer uses one GitHub Markdown request and writes all pages; shared styling
is in `site/style.css`. Commit the source, renderer/style changes and generated pages.
Offline tests check fingerprints, links, versions, colors and page navigation;
the Pages workflow refuses stale pages.

Repository **Settings → Pages → Source** must be **GitHub Actions**.
Pushes to `main` deploy automatically; the workflow can also be run manually.
The public URL is https://aurora-switch.ryanjarv.sh/.
