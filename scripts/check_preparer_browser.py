"""Offline preparer smoke: browser/reference byte parity and rejected inputs."""

import argparse
import base64
import importlib.util
import json
from pathlib import Path
import tempfile

from check_reference_browser import Browser
from prepare_bundle import assemble, digest

ROOT = Path(__file__).resolve().parents[1]


def check(endpoint, firefox=None):
    spec = importlib.util.spec_from_file_location(
        "bundle_fixture", ROOT / "tests/test_prepare_bundle.py"
    )
    fixtures = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixtures)
    template, files = fixtures.fixture()
    encoded = [base64.b64encode(data).decode() for data in files]
    source = (
        (ROOT / "scripts/bundle_preparer.html")
        .read_text()
        .replace("/* RELEASE_TEMPLATE */ null", json.dumps(template))
    )
    with tempfile.TemporaryDirectory(
        prefix="aurora-preparer-browser-", dir=ROOT / ".deps"
    ) as temporary:
        page = Path(temporary) / "prepare.html"
        page.write_text(source)
        for dark in (False, True):
            browser = Browser(endpoint, firefox, dark=dark)
            try:
                browser.open(page.as_uri())
                assert browser.execute("return window.isSecureContext && Boolean(crypto.subtle)")
                for width in (390, 1280):
                    browser.request("POST", "/window/rect", {"width": width, "height": 800})
                    assert browser.execute(
                        "return document.documentElement.scrollWidth <= innerWidth"
                    )
                    assert browser.execute('return !document.getElementById("prepare").disabled')
                script = """const done=arguments[arguments.length-1];
                    (async()=>{
                        const files=INPUT.map(text=>Uint8Array.from(atob(text),c=>c.charCodeAt(0)));
                        const output=await buildBundle(TEMPLATE,files);
                        let rejected=0;
                        for(const bad of [[],[files[1]],[files[0],files[0]],[files[0].slice(1)]]) {
                            try { await buildBundle(TEMPLATE,bad); }
                            catch { ++rejected; }
                        }
                        for (const field of ['UPDATER_BLOCK_SIZE', 'BUNDLE_DATA']) {
                            const missing=structuredClone(TEMPLATE);
                            delete missing.constants[field];
                            try { await buildBundle(missing,files); }
                            catch { ++rejected; }
                        }
                        const unsafe=TEMPLATE;
                        unsafe.constants.BUNDLE_LIMIT=0x907d0000;
                        try { await buildBundle(unsafe,files); }
                        catch { ++rejected; }
                        return {hash:[...new Uint8Array(await crypto.subtle.digest("SHA-256",output))].map(v=>v.toString(16).padStart(2,"0")).join(""),size:output.length,rejected};
                    })().then(done,error=>done({error:error.message}));""".replace(
                    "INPUT", json.dumps(encoded)
                ).replace(
                    "TEMPLATE", json.dumps(template)
                )
                result = browser.request("POST", "/execute/async", {"script": script, "args": []})
                expected = assemble(template, files)
                assert result == dict(hash=digest(expected), size=len(expected), rejected=7), result
                # Native file input and the real button exercise local File objects.
                paths = []
                for i, data in enumerate(files):
                    path = Path(temporary) / f"user-{i}.bin"
                    path.write_bytes(data)
                    paths.append(str(path))
                element = browser.request(
                    "POST", "/element", {"using": "css selector", "value": "#firmware"}
                )
                key = element["element-6066-11e4-a52e-4f735466cecf"]
                browser.request("POST", f"/element/{key}/value", {"text": "\n".join(paths)})
                intercept = 'URL.createObjectURL = blob => { blob.arrayBuffer().then(sha256).then(hash=>window.preparedHash=hash); return "blob:test"; }; HTMLAnchorElement.prototype.click=function(){window.downloadName=this.download;};'
                browser.execute(
                    'const script=document.createElement("script"); script.textContent='
                    + json.dumps(intercept)
                    + "; document.documentElement.appendChild(script);"
                )
                browser.execute('document.getElementById("firmware").focus()')
                browser.request(
                    "POST",
                    "/actions",
                    {
                        "actions": [
                            {
                                "type": "key",
                                "id": "keys",
                                "actions": [
                                    {"type": "keyDown", "value": "\ue004"},
                                    {"type": "keyUp", "value": "\ue004"},
                                ],
                            }
                        ]
                    },
                )
                browser.wait('document.activeElement.id === "prepare"')
                browser.request(
                    "POST",
                    "/actions",
                    {
                        "actions": [
                            {
                                "type": "key",
                                "id": "keys",
                                "actions": [
                                    {"type": "keyDown", "value": "\ue007"},
                                    {"type": "keyUp", "value": "\ue007"},
                                ],
                            }
                        ]
                    },
                )
                browser.wait('window.downloadName === "AuroraSwitch.bin" && window.preparedHash')
                actual = browser.execute("return window.preparedHash")
                assert actual == digest(expected)
                print(
                    "PASS offline file input, byte parity, rejection, mobile, keyboard-native controls and",
                    "dark" if dark else "light",
                )
            finally:
                browser.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--webdriver", required=True)
    parser.add_argument("--firefox", help="Firefox binary, if not found automatically")
    args = parser.parse_args()
    check(args.webdriver, args.firefox)
