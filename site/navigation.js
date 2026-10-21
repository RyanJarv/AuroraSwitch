// Enhance static pages with instant navigation; ordinary links remain the fallback.

const base = new URL('.', import.meta.url);

/** Route only known pages in this directory; leave external links to the browser. */
function pageFor(url, pages) {
    if (url.origin !== base.origin || url.search) {
        return null;
    }
    const filename = url.pathname === base.pathname
        ? 'index.html'
        : url.pathname.slice(base.pathname.length);
    return url.pathname.startsWith(base.pathname) && Object.hasOwn(pages, filename)
        ? filename
        : null;
}

/** Load the complete reference once; failures leave ordinary page links working. */
async function start() {
    const script = document.querySelector('script[data-reference]');
    const response = await fetch(new URL(script.dataset.reference, base));
    if (!response.ok) {
        throw new Error(`Reference HTTP ${response.status}`);
    }
    const data = await response.json();
    for (const field of ['reference', 'renderer', 'quickstart']) {
        const expected = document.querySelector(`meta[name="${field}-sha256"]`).content;
        if (data[`${field}_sha256`] !== expected) {
            throw new Error('Stale reference data');
        }
    }
    let current = pageFor(new URL(location.href), data.pages);
    if (!current) {
        return;
    }
    const main = document.querySelector('main');
    const links = document.querySelectorAll('.sidebar a, .mobile-menu a');
    const initialBody = main.innerHTML.trim();
    if (data.pages[current].body.trim() !== initialBody) {
        throw new Error('Reference page mismatch');
    }
    history.scrollRestoration = 'manual';

    /** Replace the page content inside the existing layout and navigation. */
    function show(filename) {
        current = filename;
        const page = data.pages[filename];
        main.innerHTML = page.body;
        document.body.dataset.page = filename;
        for (const link of links) {
            if (link.getAttribute('href') === filename) {
                link.setAttribute('aria-current', 'page');
            } else {
                link.removeAttribute('aria-current');
            }
        }
        document.title = page.title;
    }

    /** Announce page changes to screen readers and restore the scroll position. */
    function position(url, saved) {
        requestAnimationFrame(() => {
            const heading = main.querySelector('h1, h2') || main;
            heading.setAttribute('tabindex', '-1');
            heading.focus({ preventScroll: true });
            let target;
            try {
                target = document.getElementById(decodeURIComponent(url.hash.slice(1)));
            } catch {
                // A malformed section link should not prevent opening the page.
            }
            if (saved) {
                window.scrollTo(saved.x, saved.y);
            } else if (target) {
                target.scrollIntoView();
            } else {
                window.scrollTo(0, 0);
            }
        });
    }

    // Leave the initial HTML and its native fragment handling untouched.
    document.documentElement.dataset.navigation = 'spa';

    document.addEventListener('click', event => {
        if (event.defaultPrevented || event.button !== 0 || event.metaKey || event.ctrlKey
            || event.shiftKey || event.altKey) {
            return;
        }
        const link = event.target.closest('a[href]');
        if (!link || link.hasAttribute('download') || (link.target && link.target !== '_self')) {
            return;
        }
        const url = new URL(link.href);
        const filename = pageFor(url, data.pages);
        if (!filename || (filename === current && url.hash)) {
            return;
        }
        event.preventDefault();
        history.replaceState({
            ...history.state,
            referenceScroll: { x: scrollX, y: scrollY }
        }, '', location.href);
        history.pushState(null, '', url);
        show(filename);
        position(url);
    });

    window.addEventListener('popstate', event => {
        const url = new URL(location.href);
        const filename = pageFor(url, data.pages);
        if (!filename) {
            location.reload();
            return;
        }
        show(filename);
        position(url, event.state?.referenceScroll);
    });
}

start().catch(error => {
    document.documentElement.dataset.navigation = 'static';
    console.warn('Reference SPA unavailable; using ordinary page links.', error);
});
