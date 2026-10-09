#!/usr/bin/env python3
"""Smoke-test SPA navigation and fallbacks against a running Firefox WebDriver."""

import argparse
from collections import Counter
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
import time
from urllib.parse import urlsplit
from urllib.request import Request, urlopen

SITE = Path(__file__).resolve().parents[1] / "site"


class SiteHandler(SimpleHTTPRequestHandler):
    """Serve real assets, with explicit unavailable/stale-data negative controls."""

    counts = Counter()
    fault = None

    def log_message(self, *_):
        pass

    def end_headers(self):
        # Negative controls must not accidentally reuse a previous healthy response.
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def do_GET(self):
        path = urlsplit(self.path).path
        self.counts[path] += 1
        if path == "/reference.json" and self.fault:
            if self.fault == "unavailable":
                self.send_error(503)
                return
            data = json.loads((SITE / "reference.json").read_text())
            field = "quickstart_sha256" if self.fault == "stale-quickstart" else "renderer_sha256"
            data[field] = "stale"
            body = json.dumps(data).encode()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return
        super().do_GET()


class Browser:
    """Use the standard WebDriver HTTP API; no additional Python packages needed."""

    def __init__(self, endpoint, firefox, javascript=True, dark=False):
        self.endpoint = endpoint.rstrip("/")
        # Firefox's content override is 0 for dark and 1 for light. Pin it so
        # screenshots and contrast checks do not depend on the host's theme.
        options = {"args": ["-headless"], "prefs": {
            "javascript.enabled": javascript,
            "layout.css.prefers-color-scheme.content-override": 0 if dark else 1,
        }}
        if firefox:
            options["binary"] = firefox
        value = self.request("POST", "/session", {
            "capabilities": {"alwaysMatch": {"browserName": "firefox", "moz:firefoxOptions": options}},
        })
        self.endpoint += f'/session/{value["sessionId"]}'

    def request(self, method, path, data=None):
        body = json.dumps(data).encode() if data is not None else None
        request = Request(self.endpoint + path, data=body, method=method, headers={"Content-Type": "application/json"})
        with urlopen(request, timeout=30) as response:
            return json.load(response)["value"]

    def execute(self, script):
        return self.request("POST", "/execute/sync", {"script": script, "args": []})

    def open(self, url):
        self.request("POST", "/url", {"url": url})

    def click(self, selector):
        element = self.request("POST", "/element", {"using": "css selector", "value": selector})
        self.request("POST", f'/element/{element["element-6066-11e4-a52e-4f735466cecf"]}/click', {})

    def choose(self, filename):
        """Use the visible directory: home cards or a firmware-page sidebar."""
        home = self.execute("return document.body.dataset.page === 'index.html'")
        self.click(f'{".category-overview" if home else ".sidebar"} a[href="{filename}"]')

    def wait(self, expression):
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            if self.execute(f"return Boolean({expression})"):
                return
            time.sleep(0.05)
        raise AssertionError(f"Browser condition timed out: {expression}")

    def close(self):
        self.request("DELETE", "")


def check(args, base):
    """Verify real clicks, history, deep links, responsive views and native fallback."""
    browser = Browser(args.webdriver, args.firefox)
    try:
        browser.open(base + "index.html")
        browser.wait("document.documentElement.dataset.navigation === 'spa'")
        browser.request("POST", "/window/rect", {"width": 1280, "height": 600})
        assert browser.execute("""const link = document.querySelector('.category-overview a[href="fdn-122.html"]');
            const name = link.firstElementChild;
            const color = link.querySelector('.color-label');
            const dot = color.querySelector('.swatch');
            const titleBounds = name.getBoundingClientRect(), colorBounds = color.getBoundingClientRect();
            return name.textContent === 'FDN' && color.textContent === 'Blue'
                && getComputedStyle(dot).backgroundColor === 'rgb(55, 135, 255)'
                && colorBounds.left >= titleBounds.right
                && Math.abs(colorBounds.top + colorBounds.height / 2 - titleBounds.top - titleBounds.height / 2) < 2;""")
        assert browser.execute("""const groups = [...document.querySelectorAll('.category-overview .category-group')];
            return groups.map(group => group.querySelector('h2').textContent).join('|')
                === 'ReverbsBlue|DelaysGreen|Synths / OtherAmber'
                && groups.map(group => getComputedStyle(group).borderTopColor).join('|')
                === 'rgb(55, 135, 255)|rgb(53, 191, 88)|rgb(220, 154, 37)'
                && groups.map(group => group.querySelectorAll('li').length).join(',') === '4,3,4'
                && [...document.querySelectorAll('.selector-controls kbd')].map(key => key.textContent).join(',')
                === 'Shift,Reverse,Freeze'
                && [...document.querySelectorAll('.selector-controls dd')].map(action => action.textContent).join('|')
                === 'Next category|Next firmware|Verify & launch'
                && !document.querySelector('details, summary')
                && document.querySelector('.setup-guide h2').getClientRects().length > 0
                && getComputedStyle(document.querySelector('.selector-controls')).borderBottomWidth === '0px'
                && getComputedStyle(document.querySelector('.site-header')).borderBottomWidth === '1px'
                && groups.every(group => group.getBoundingClientRect().width > 250);""")
        browser.execute("""window.referenceTestMarker = true;
            window.referenceLayout = document.querySelector('.layout');
            window.referenceMain = document.querySelector('main');
            window.referenceSidebar = document.querySelector('.sidebar');
            window.referenceMobileMenu = document.querySelector('.mobile-menu');
            window.referenceFooter = document.querySelector('footer');
            window.referenceHomeScroll = Math.min(150, document.documentElement.scrollHeight - innerHeight);
            window.scrollTo(0, referenceHomeScroll);""")
        browser.wait("scrollY === referenceHomeScroll")
        before = SiteHandler.counts.copy()
        browser.choose('fdn-122.html')
        browser.wait("location.pathname.endsWith('/fdn-122.html') && document.activeElement.tagName === 'H1'")
        assert browser.execute("return window.referenceTestMarker && document.title.startsWith('FDN')")
        assert browser.execute("""return referenceLayout === document.querySelector('.layout')
            && referenceMain === document.querySelector('main')
            && referenceSidebar === document.querySelector('.sidebar')
            && referenceMobileMenu === document.querySelector('.mobile-menu')
            && referenceFooter === document.querySelector('footer')
            && [...document.querySelectorAll('[aria-current="page"]')]
                .every(link => link.getAttribute('href') === 'fdn-122.html')
            && document.querySelectorAll('[aria-current="page"]').length === 2;""")
        browser.wait("document.querySelector('.panel-map img')?.complete && document.querySelector('.panel-map img').naturalWidth > 0")
        # The shared drawing may load on the first control view; no page/data reload.
        assert set((SiteHandler.counts - before)) <= {"/aurora-panel.svg"}, "SPA navigation reloaded a document or data"
        assert browser.execute("""const heading = document.querySelector('#details-and-sources');
            return heading.tagName === 'H2' && heading.nextElementSibling.getClientRects().length > 0;""")
        browser.request("POST", "/back", {})
        browser.wait("document.title.startsWith('Firmware reference') && scrollY === referenceHomeScroll && document.activeElement.tagName === 'H2'")
        browser.request("POST", "/forward", {})
        browser.wait("document.title.startsWith('FDN')")
        assert browser.execute("return !document.querySelector('.page-links')")
        browser.click('.sidebar a[href="morse.html"]')
        browser.wait("document.title.startsWith('Morse')")
        browser.request("POST", "/back", {})
        browser.wait("document.title.startsWith('FDN')")
        browser.request("POST", "/forward", {})
        browser.wait("document.title.startsWith('Morse')")
        data = json.loads((SITE / "reference.json").read_text())
        before = SiteHandler.counts.copy()
        for filename, page in data["pages"].items():
            browser.choose(filename)
            browser.wait(f"document.title === {json.dumps(page['title'])}")
            assert browser.execute("return window.referenceTestMarker")
            assert browser.execute(f"""const selected = [...document.querySelectorAll('[aria-current="page"]')];
                return selected.length === {3 if filename == 'index.html' else 2}
                    && selected.every(link => link.getAttribute('href') === '{filename}')
                    && document.body.dataset.page === '{filename}';""")
        assert set((SiteHandler.counts - before)) <= {"/aurora-panel.svg"}, "A firmware view caused a document/data reload"
        # Direct URLs still render their own fallback, then enhance the deep link.
        browser.open(base + "tempest-100.html#buttons-and-gates")
        browser.wait("document.documentElement.dataset.navigation === 'spa'")
        assert browser.execute("return document.querySelector('h1').textContent === 'Tempest 1.0.0'")
        browser.open(base + "index.html")
        browser.wait("document.documentElement.dataset.navigation === 'spa'")
        # Firefox constrains outer window size; iframes provide exact CSS viewports.
        for width in (320, 390, 760, 1024):
            browser.execute(f"""
                document.querySelector('#responsive-check')?.remove();
                const frame = document.createElement('iframe');
                frame.id = 'responsive-check'; frame.style.width = '{width}px';
                frame.style.height = '800px'; frame.src = 'fdn-122.html';
                document.body.append(frame);
            """)
            browser.wait("document.querySelector('iframe')?.contentDocument?.documentElement?.dataset.navigation === 'spa'")
            assert browser.execute("""const frame = document.querySelector('iframe');
                return frame.contentDocument.documentElement.scrollWidth <= frame.contentWindow.innerWidth;"""), width
            browser.wait("document.querySelector('iframe')?.contentDocument?.querySelector('.panel-map img')?.naturalWidth > 0")
            for page in ('index.html', 'flux-capacitor.html'):
                browser.execute(f"document.querySelector('iframe').src = '{page}'")
                browser.wait(f"document.querySelector('iframe')?.contentWindow?.location.pathname.endsWith('/{page}') && document.querySelector('iframe')?.contentDocument?.documentElement?.dataset.navigation === 'spa'")
                assert browser.execute("""const frame = document.querySelector('iframe');
                    return frame.contentDocument.documentElement.scrollWidth <= frame.contentWindow.innerWidth;"""), (width, page)
                if page == 'index.html':
                    assert browser.execute("""const frame = document.querySelector('iframe');
                        const cards = [...frame.contentDocument.querySelectorAll('.category-overview .category-group')];
                        return cards.length === 3 && cards.every(card => card.getBoundingClientRect().width > 0)
                            && getComputedStyle(frame.contentDocument.querySelector('.sidebar')).display === 'none';"""), width
        browser.execute("document.querySelector('iframe').style.width = '390px'")
        assert browser.execute("""const doc = document.querySelector('iframe').contentDocument;
            return doc.querySelector('.mobile-menu').getClientRects().length > 0
                && !doc.querySelector('details, summary');""")
        browser.execute("document.querySelector('iframe').contentDocument.querySelector('.mobile-menu a[href=\"morse.html\"]').click()")
        browser.wait("document.querySelector('iframe').contentDocument.title.startsWith('Morse')")
        assert browser.execute("return !document.querySelector('iframe').contentDocument.querySelector('details, summary')")
        browser.open(base + "index.html")
        browser.wait("document.documentElement.dataset.navigation === 'spa'")
        # A physical keyboard can activate the same real link, without a custom widget.
        browser.execute("document.querySelector('.category-overview a[href=\"fdn-122.html\"]').focus()")
        browser.request("POST", "/actions", {"actions": [{"type": "key", "id": "keyboard", "actions": [
            {"type": "keyDown", "value": "\ue007"}, {"type": "keyUp", "value": "\ue007"}]}]})
        browser.wait("document.title.startsWith('FDN') && document.activeElement.tagName === 'H1'")
        for fault in ("unavailable", "stale", "stale-quickstart"):
            SiteHandler.fault = fault
            browser.open(base + "index.html")
            browser.wait("document.documentElement.dataset.navigation === 'static'")
            assert not browser.execute("return document.documentElement.dataset.navigation === 'spa'")
            browser.choose('fdn-122.html')
            assert browser.execute("return document.querySelector('h1').textContent === 'FDN 1.2.2'")
            print(f"PASS {fault} SPA data falls back to ordinary pages")
    finally:
        SiteHandler.fault = None
        browser.close()
    browser = Browser(args.webdriver, args.firefox, javascript=False)
    try:
        browser.open(base + "index.html")
        # JS is disabled, so query only native WebDriver elements.
        browser.click('.category-overview a[href="fdn-122.html"]')
        element = browser.request("POST", "/element", {"using": "css selector", "value": "h1"})
        heading = browser.request("GET", f'/element/{element["element-6066-11e4-a52e-4f735466cecf"]}/text')
        assert heading == "FDN 1.2.2"
        print("PASS JavaScript-disabled navigation")
    finally:
        browser.close()
    browser = Browser(args.webdriver, args.firefox, dark=True)
    try:
        browser.open(base + "index.html")
        browser.wait("document.documentElement.dataset.navigation === 'spa'")
        assert browser.execute("return matchMedia('(prefers-color-scheme: dark)').matches")
        assert browser.execute("return getComputedStyle(document.body).backgroundColor === 'rgb(19, 24, 32)'")
        browser.choose('flux-capacitor.html')
        browser.wait("document.title.startsWith('Flux Capacitor')")
        assert browser.execute("""return document.querySelectorAll('.firmware-meta .swatch').length === 2
            && document.querySelector('.firmware-meta .category-label').textContent === 'Delays'
            && document.querySelector('.firmware-meta .color-label').textContent === 'Yellow';""")
        print("PASS dark-theme directory and controls")
    finally:
        browser.close()
    print("PASS SPA clicks (no document/data reload), panel image, visible details, history, direct links and mobile layouts")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--webdriver", required=True, help="Running geckodriver URL")
    parser.add_argument("--firefox", help="Firefox executable when not found automatically")
    args = parser.parse_args()
    server = ThreadingHTTPServer(("127.0.0.1", 0), partial(SiteHandler, directory=str(SITE)))
    threading.Thread(target=server.serve_forever, daemon=True).start()
    try:
        check(args, f"http://127.0.0.1:{server.server_port}/")
    finally:
        server.shutdown()
        server.server_close()
