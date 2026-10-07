#!/usr/bin/env python3
"""Generate SPA content and direct-link HTML fallbacks from one reference."""

from dataclasses import dataclass
import hashlib
import html
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "docs/firmware_reference.md"
REPOSITORY = "https://github.com/RyanJarv/AuroraSwitch/blob/main/"
COLORS = {
    "Blue": "#3787ff", "Green": "#35bf58", "Cyan": "#32c9ce",
    "Magenta": "#d855d5", "Amber": "#dc9a25", "Yellow": "#ead342",
    "White": "#fff", "Pale red": "#ec9292", "Azure": "#3ea7dc",
}
PANEL_CONTROLS = ("Warp", "Time", "Blur", "Reflect", "Mix", "Atmosphere", "Reverse", "Freeze", "Shift")


def label_controls(content: str) -> str:
    """Number table control names against the drawing without duplicating functions."""
    def label(match: re.Match) -> str:
        cell = match[1]
        text = html.unescape(re.sub(r"<[^>]+>", "", cell))
        names = re.findall(r"\b(?:" + "|".join(PANEL_CONTROLS) + r")\b", text)
        numbers = [str(PANEL_CONTROLS.index(name) + 1) for name in dict.fromkeys(names)]
        if not numbers:
            return match[0]
        badges = "".join(f'<span class="control-number">{number}</span>' for number in numbers)
        return f'<tr><td>{badges} {cell}</td>'

    return re.sub(r"<tr>\s*<td>(.*?)</td>", label, content, flags=re.S)


def slug(heading: str) -> str:
    """Match the reference's GitHub heading links, including doubled hyphens."""
    return re.sub(r"[^\w\- ]", "", html.unescape(heading).lower()).replace(" ", "-")


@dataclass(frozen=True)
class Section:
    """A source section owns its stable legacy anchor and generated page."""

    heading: str
    markdown: str

    @property
    def anchor(self) -> str:
        return slug(self.heading)

    @property
    def title(self) -> str:
        return self.heading.split(" — ")[0]

    @property
    def filename(self) -> str:
        if self.heading == "Color lookup":
            return "index.html"
        return f"{slug(self.title)}.html"


@dataclass(frozen=True)
class Entry:
    """One version/color row; versions can share a firmware control page."""

    color: str
    name: str
    anchor: str
    description: str
    availability: str


def read_reference(source: str) -> tuple[str, list[Section], list[Entry]]:
    """Use the color table as the only navigation catalog; reject stale links."""
    parts = re.split(r"^## (.+)\n", source, flags=re.M)
    sections = [Section(parts[i], parts[i + 1]) for i in range(1, len(parts), 2)]
    by_anchor = {section.anchor: section for section in sections}
    if len(by_anchor) != len(sections):
        raise ValueError("Duplicate reference section")
    if len({section.filename for section in sections if section.filename != "index.html"}) != len(sections) - 1:
        raise ValueError("Duplicate reference page")
    for required in ("color-lookup", "outside-the-selector-catalog"):
        if required not in by_anchor:
            raise ValueError(f"Missing reference section: {required}")
    entries = []
    for line in by_anchor["color-lookup"].markdown.splitlines():
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if not line.startswith("|") or cells[0] in ("Selector color", "---"):
            continue
        if len(cells) != 4:
            raise ValueError(f"Malformed color row: {line}")
        color, link, description, availability = cells
        match = re.fullmatch(r"\[([^]]+)\]\(#([\w-]+)\)", link)
        if color not in COLORS or availability not in ("Release", "Development") or not match:
            raise ValueError(f"Invalid color row: {line}")
        if match[2] not in by_anchor or by_anchor[match[2]].filename == "index.html":
            raise ValueError(f"Missing firmware section: {link}")
        entries.append(Entry(color, match[1], match[2], description, availability))
    if not entries:
        raise ValueError("Missing selector colors")
    # Versions share one family page/color; unrelated families cannot share it.
    for entry in entries:
        for other in entries:
            if (entry.anchor == other.anchor) != (entry.color == other.color):
                raise ValueError("Inconsistent family or duplicate selector colors")
    if len({entry.name for entry in entries}) != len(entries):
        raise ValueError("Duplicate firmware version")
    unused = set(by_anchor) - {entry.anchor for entry in entries} - {
        "color-lookup", "outside-the-selector-catalog",
    }
    if unused:
        raise ValueError(f"Unlinked firmware sections: {sorted(unused)}")
    return parts[0], sections, entries


def render_markdown(source: str, sections: list[Section]) -> str:
    """Render once with GitHub GFM; link source sections to their static pages."""
    by_anchor = {section.anchor: section for section in sections}

    def absolute_link(match: re.Match) -> str:
        target = match[1]
        if target.startswith("#") and target[1:] in by_anchor:
            section = by_anchor[target[1:]]
            return f"]({section.filename}#{section.anchor})"
        if target.startswith(("#", "https://", "http://")):
            return match[0]
        file, separator, anchor = target.partition("#")
        relative = (SOURCE.parent / file).resolve().relative_to(ROOT)
        return f"]({REPOSITORY}{relative.as_posix()}{separator}{anchor})"

    source = re.sub(r"\]\(([^)]+)\)", absolute_link, source)
    response = subprocess.run(
        ["gh", "api", "markdown", "--input", "-"],
        input=json.dumps({"text": source, "mode": "gfm"}),
        text=True, capture_output=True, check=True,
    )
    return re.sub(
        r"<h([1-6])>(.*?)</h\1>",
        lambda m: f'<h{m[1]} id="{slug(m[2])}">{m[2]}</h{m[1]}>',
        response.stdout, flags=re.S,
    )


def swatch(color: str) -> str:
    """Always accompany the visual LED approximation with a written color name."""
    return f'<span class="swatch" style="background:{COLORS[color]}" aria-hidden="true"></span>'


def navigation(sections: list[Section], entries: list[Entry], current: str) -> str:
    """Share the same directory between the desktop sidebar and mobile menu."""
    parts = ['<a class="index-link" href="index.html">All firmware &amp; colors</a>']
    for availability, label in (("Release", "Release firmware"), ("Development", "Development additions")):
        parts.append(f'<h2>{label}</h2><ul>')
        for section in sections:
            versions = [entry for entry in entries if entry.anchor == section.anchor]
            if not versions or ("Release" if any(entry.availability == "Release" for entry in versions) else "Development") != availability:
                continue
            active = ' aria-current="page"' if current == section.filename else ""
            colors = versions[0].color
            parts.append(f'<li><a href="{section.filename}"{active}><span>{html.escape(section.title)}</span>'
                         f'<small>{html.escape(colors)}</small></a></li>')
        parts.append('</ul>')
    parts.append('<a class="index-link" href="outside-the-selector-catalog.html">Other firmware / limitations</a>')
    return "\n".join(parts)


def document(title: str, body: str, menu: str, digest: str) -> str:
    """Render immediately; locally served Preact enhances navigation afterward."""
    renderer_digest = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    app_digest = hashlib.sha256((ROOT / "site/app.js").read_bytes()).hexdigest()
    return f'''<!doctype html>
<!-- Generated by scripts/render_reference.py; edit docs/firmware_reference.md. -->
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="reference-sha256" content="{digest}">
<meta name="renderer-sha256" content="{renderer_digest}">
<meta name="description" content="AuroraSwitch firmware colors, knobs, buttons, gates and modes.">
<title>{html.escape(title)} · AuroraSwitch</title>
<link rel="stylesheet" href="style.css">
<script type="module" src="app.js?v={app_digest}" data-reference="reference.json?v={digest}-{renderer_digest}"></script>
</head>
<body>
<a class="skip-link" href="#main">Skip to controls</a>
<header class="site-header">
<a class="brand" href="index.html">AuroraSwitch <span>Firmware reference</span></a>
<nav aria-label="Site navigation">
<a href="{REPOSITORY}docs/user_guide.md">Setup guide</a>
<a href="https://github.com/RyanJarv/AuroraSwitch">GitHub</a>
</nav>
</header>
<div class="layout">
<aside class="sidebar"><nav aria-label="Firmware directory">{menu}</nav></aside>
<div class="content">
<details class="mobile-menu"><summary>Choose firmware / color</summary>
<nav aria-label="Mobile firmware directory">{menu}</nav></details>
<main id="main">
{body}
</main>
<footer><a href="{REPOSITORY}docs/firmware_reference.md">Markdown reference</a></footer>
</div>
</div>
</body>
</html>
'''


def render_pages() -> dict[str, str]:
    """Keep one source of control facts while generating short, direct-link pages."""
    source = SOURCE.read_text()
    _, sections, entries = read_reference(source)
    digest = hashlib.sha256(SOURCE.read_bytes()).hexdigest()
    rendered = render_markdown(source, sections)
    parts = re.split(r'(<h2 id="[^"]+">.*?</h2>)', rendered, flags=re.S)
    if len(parts) != 1 + 2 * len(sections):
        raise ValueError("Rendered sections do not match Markdown sections")
    bodies = {section.anchor: parts[2 * i + 2] for i, section in enumerate(sections)}
    by_anchor = {section.anchor: section for section in sections}
    home = '''<h1>Find your firmware</h1>
<p class="lead">Choose by name or match the selector’s Reverse LED color.</p>
<p class="launch-line">Reverse selects → Freeze verifies → wait for <strong>Freeze green</strong> → Shift launches.</p>
'''
    for availability, label, note in (
        ("Release", "Release firmware", "Published selector."),
        ("Development", "Development builds", "Not in the published selector."),
    ):
        anchor = "color-lookup" if availability == "Release" else "development-builds"
        home += f'<section aria-labelledby="{anchor}"><h2 id="{anchor}">{label}</h2><p>{note}</p><div class="cards">'
        for entry in entries:
            if entry.availability != availability:
                continue
            section = by_anchor[entry.anchor]
            # Preserve old index fragments at the first card for each firmware family.
            legacy = f' id="{entry.anchor}"' if entry == next(e for e in entries if e.anchor == entry.anchor) else ""
            home += (f'<a class="firmware-card" href="{section.filename}"{legacy}>'
                     f'<span class="color-label">{swatch(entry.color)}{entry.color}</span>'
                     f'<h3>{html.escape(entry.name)}</h3><p>{html.escape(entry.description)}</p>'
                     '<span class="card-action">View controls →</span></a>\n')
        home += '</div></section>\n'
    home += '''<p class="other-firmware"><a href="outside-the-selector-catalog.html">Other firmware and current limitations →</a></p>
'''
    pages = {"index.html": document("Firmware reference", home, navigation(sections, entries, "index.html"), digest)}
    for section in sections:
        if section.filename == "index.html":
            continue
        versions = [entry for entry in entries if entry.anchor == section.anchor]
        badges = "".join(f'<span class="version-label">{swatch(entry.color)}{html.escape(entry.name)} · {entry.color}'
                         f'{" · development" if entry.availability == "Development" else ""}</span>' for entry in versions)
        content = bodies[section.anchor]
        # Each page starts at h1; source h3 control groups become its h2 landmarks.
        content = re.sub(r'<(/?)h3\b', r'<\1h2', content)
        # Keep lookup controls visible; secondary details stay one click away.
        content = content.replace('<h2 id="details-and-sources">Details and sources</h2>',
                                  '<details class="reference-details" id="details-and-sources"><summary>Details &amp; sources</summary>')
        if 'class="reference-details"' in content:
            content += '</details>'
        content = label_controls(content)
        body = f'<h1 id="{section.anchor}">{html.escape(section.title)}</h1>'
        if badges:
            body += f'<div class="version-labels" aria-label="Versions and selector colors">{badges}</div>'
        if badges:
            # The original drawing maps physical positions; all functions remain in Markdown.
            intro, separator, controls = content.partition('<h2 ')
            body += intro
            body += '''<div class="control-layout"><figure class="panel-map">
<img src="aurora-panel.svg" width="280" height="580" alt="Aurora panel: knobs 1 Warp, 2 Time, 3 Blur, 4 Reflect, 5 Mix, 6 Atmosphere; buttons 7 Reverse, 8 Freeze, 9 Shift.">
</figure><div class="control-tables">'''
            body += separator + controls + '</div></div>'
        else:
            body += content
        pages[section.filename] = document(section.title, body, navigation(sections, entries, section.filename), digest)
    return pages


def reference_data(pages: dict[str, str]) -> dict:
    """Reuse generated bodies verbatim; never maintain a second controls source."""
    data = {}
    for filename, page in pages.items():
        title = re.search(r"<title>(.*?)</title>", page, re.S)[1]
        body = re.search(r'<main id="main">\n(.*?)\n</main>', page, re.S)[1]
        data[filename] = {"title": html.unescape(title), "body": body}
    _, sections, entries = read_reference(SOURCE.read_text())
    return {
        "reference_sha256": hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        "renderer_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "menu": navigation(sections, entries, ""),
        "pages": data,
    }


if __name__ == "__main__":
    try:
        pages = render_pages()
        for filename, page in pages.items():
            (ROOT / "site" / filename).write_text(page)
        (ROOT / "site/reference.json").write_text(json.dumps(reference_data(pages), ensure_ascii=False, indent=2) + "\n")
        print(f"Updated SPA content and {len(pages)} direct-link fallbacks.")
    except subprocess.CalledProcessError as error:
        sys.stderr.write(error.stderr)
        raise SystemExit(error.returncode) from error
