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
            data["renderer_sha256"] = "stale"
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

    def __init__(self, endpoint, firefox, javascript=True):
        self.endpoint = endpoint.rstrip("/")
        options = {"args": ["-headless"], "prefs": {"javascript.enabled": javascript}}
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
        browser.execute("window.referenceTestMarker = true; window.scrollTo(0, 150)")
        browser.wait("scrollY === 150")
        before = SiteHandler.counts.copy()
        browser.click('.firmware-card[href="fdn-122.html"]')
        browser.wait("location.pathname.endsWith('/fdn-122.html') && document.activeElement.tagName === 'H1'")
        assert browser.execute("return window.referenceTestMarker && document.title.startsWith('FDN')")
        browser.wait("document.querySelector('.panel-map img')?.complete && document.querySelector('.panel-map img').naturalWidth > 0")
        # The shared drawing may load on the first control view; no page/data reload.
        assert set((SiteHandler.counts - before)) <= {"/aurora-panel.svg"}, "SPA navigation reloaded a document or data"
        browser.click('.reference-details summary')
        assert browser.execute("return document.querySelector('.reference-details').open")
        browser.click('.reference-details summary')
        assert browser.execute("return !document.querySelector('.reference-details').open")
        browser.request("POST", "/back", {})
        browser.wait("document.title.startsWith('Firmware reference') && scrollY === 150")
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
            browser.click(f'.sidebar a[href="{filename}"]')
            browser.wait(f"document.title === {json.dumps(page['title'])}")
            assert browser.execute("return window.referenceTestMarker")
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
        browser.execute("""const frame = document.querySelector('iframe'); frame.style.width = '390px';
            frame.contentDocument.querySelector('.mobile-menu summary').click();""")
        browser.wait("document.querySelector('iframe').contentDocument.querySelector('.mobile-menu').open")
        browser.execute("document.querySelector('iframe').contentDocument.querySelector('.mobile-menu a[href=\"morse.html\"]').click()")
        browser.wait("document.querySelector('iframe').contentDocument.title.startsWith('Morse')")
        assert browser.execute("return !document.querySelector('iframe').contentDocument.querySelector('.mobile-menu').open")
        for fault in ("unavailable", "stale"):
            SiteHandler.fault = fault
            browser.open(base + "index.html")
            browser.wait("document.documentElement.dataset.navigation === 'static'")
            assert not browser.execute("return document.documentElement.dataset.navigation === 'spa'")
            browser.click('.firmware-card[href="fdn-122.html"]')
            assert browser.execute("return document.querySelector('h1').textContent === 'FDN 1.2.2'")
            print(f"PASS {fault} SPA data falls back to ordinary pages")
    finally:
        SiteHandler.fault = None
        browser.close()
    browser = Browser(args.webdriver, args.firefox, javascript=False)
    try:
        browser.open(base + "index.html")
        browser.click('.firmware-card[href="fdn-122.html"]')
        element = browser.request("POST", "/element", {"using": "css selector", "value": "h1"})
        heading = browser.request("GET", f'/element/{element["element-6066-11e4-a52e-4f735466cecf"]}/text')
        assert heading == "FDN 1.2.2"
        print("PASS JavaScript-disabled navigation")
    finally:
        browser.close()
    print("PASS SPA clicks (no document/data reload), panel image, details, history, direct links and mobile layouts")


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
