# Reference site

GitHub Pages serves `site/index.html` directly: no Jekyll, JavaScript, dependency
installation or website build. Only `site/` is uploaded, not firmware or evidence.

Edit `docs/firmware_reference.md`, then refresh the HTML using GitHub's Markdown
renderer (requires authenticated `gh` and network):

```sh
make reference-html
make test
```

Commit both files. The offline test checks the source fingerprint, anchors and
color swatches; the Pages workflow refuses a stale HTML copy.

Repository **Settings → Pages → Source** must be **GitHub Actions**.
Pushes to `main` deploy automatically; the workflow can also be run manually.
The public URL is https://RyanJarv.github.io/AuroraSwitch/.
