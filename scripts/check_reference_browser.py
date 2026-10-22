#!/usr/bin/env python3
"""Smoke-test static reference pages against a running Firefox WebDriver."""

import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
from pathlib import Path
import threading
import time
from urllib.request import Request, urlopen

SITE = Path(__file__).resolve().parents[1] / "site"


class SiteHandler(SimpleHTTPRequestHandler):
    def log_message(self, *_):
        pass


class Browser:
    """Use the standard WebDriver HTTP API; no additional Python packages needed."""

    def __init__(self, endpoint, firefox, javascript=True, dark=False):
        self.endpoint = endpoint.rstrip("/")
        # Firefox's content override is 0 for dark and 1 for light. Pin it so
        # screenshots and contrast checks do not depend on the host's theme.
        options = {
            "args": ["-headless"],
            "prefs": {
                "javascript.enabled": javascript,
                "layout.css.prefers-color-scheme.content-override": 0 if dark else 1,
            },
        }
        if firefox:
            options["binary"] = firefox
        value = self.request(
            "POST",
            "/session",
            {
                "capabilities": {
                    "alwaysMatch": {"browserName": "firefox", "moz:firefoxOptions": options}
                },
            },
        )
        self.endpoint += f'/session/{value["sessionId"]}'

    def request(self, method, path, data=None):
        body = json.dumps(data).encode() if data is not None else None
        request = Request(
            self.endpoint + path,
            data=body,
            method=method,
            headers={"Content-Type": "application/json"},
        )
        with urlopen(request, timeout=30) as response:
            return json.load(response)["value"]

    def execute(self, script):
        return self.request("POST", "/execute/sync", {"script": script, "args": []})

    def open(self, url):
        self.request("POST", "/url", {"url": url})

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
    for javascript, dark in ((True, False), (False, False), (True, True)):
        browser = Browser(args.webdriver, args.firefox, javascript=javascript, dark=dark)
        try:
            browser.open(base + "index.html")
            browser.wait(
                "document.querySelectorAll('.category-overview .category-group').length === 3"
            )
            # Activate the ordinary link with real keyboard input.
            browser.execute(
                "document.querySelector('.category-overview a[href=\"fdn-122.html\"]').focus()"
            )
            browser.request(
                "POST",
                "/actions",
                {
                    "actions": [
                        {
                            "type": "key",
                            "id": "keyboard",
                            "actions": [
                                {"type": "keyDown", "value": "\ue007"},
                                {"type": "keyUp", "value": "\ue007"},
                            ],
                        }
                    ]
                },
            )
            browser.wait("document.title.startsWith('FDN')")
            browser.wait("document.querySelector('.panel-map img').naturalWidth > 0")
            assert browser.execute("return !document.querySelector('details, summary, script')")
            browser.request("POST", "/back", {})
            browser.wait("document.title.startsWith('Firmware reference')")
            browser.request("POST", "/forward", {})
            browser.wait("document.title.startsWith('FDN')")
            # Iframes exercise real narrow layout even with a desktop driver.
            for width in (320, 390):
                for page in ("index.html", "fdn-122.html", "flux-capacitor.html"):
                    browser.execute(
                        f"""document.querySelector('iframe')?.remove();
                        const frame = document.createElement('iframe');
                        frame.style.width = '{width}px'; frame.style.height = '900px';
                        frame.src = '{page}'; document.body.append(frame);"""
                    )
                    browser.wait(
                        "document.querySelector('iframe').contentDocument?.body?.dataset.page === '"
                        + page
                        + "'"
                    )
                    assert browser.execute(
                        """const frame = document.querySelector('iframe');
                        return frame.contentDocument.documentElement.scrollWidth <= frame.contentWindow.innerWidth;
                    """
                    ), (width, page)
            browser.open(base + "morse.html")
            browser.wait("document.title.startsWith('Morse')")
            assert browser.execute(
                "return document.querySelectorAll('[aria-current=\"page\"]').length === 2"
            )
            assert (
                browser.execute("return matchMedia('(prefers-color-scheme: dark)').matches") == dark
            )
            if dark:
                assert browser.execute(
                    "return getComputedStyle(document.body).backgroundColor === 'rgb(19, 24, 32)'"
                )
            print(
                f"PASS static links, keyboard, history, mobile, direct pages: javascript={javascript}, dark={dark}"
            )
        finally:
            browser.close()


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
