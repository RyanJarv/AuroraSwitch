// Preact owns the reference view; native URLs keep direct links and no-JS access.
import { h, render } from './vendor/preact.module.js';

const base = new URL('.', import.meta.url);

/** Limit SPA routing to known pages in this directory, never external links. */
function pageFor(url, pages) {
    if (url.origin !== base.origin || url.search) return null;
    const filename = url.pathname === base.pathname ? 'index.html' : url.pathname.slice(base.pathname.length);
    return url.pathname.startsWith(base.pathname) && Object.hasOwn(pages, filename) ? filename : null;
}

/** Retain generated HTML and accessibility landmarks rather than duplicating GFM. */
function ReferenceView({ page, filename, menu, footer }) {
    const selectedMenu = menu.replaceAll(`href="${filename}"`, `href="${filename}" aria-current="page"`);
    const directory = label => h('nav', { 'aria-label': label, dangerouslySetInnerHTML: { __html: selectedMenu } });
    return [
        h('aside', { class: 'sidebar' }, directory('Firmware directory')),
        h('div', { class: 'content' },
            h('details', { class: 'mobile-menu', key: filename },
                h('summary', null, 'Choose firmware / color'),
                directory('Mobile firmware directory')),
            h('main', { id: 'main', key: filename, dangerouslySetInnerHTML: { __html: page.body } }),
            h('footer', { dangerouslySetInnerHTML: { __html: footer } })),
    ];
}

/** Load the complete reference once; any bootstrap failure leaves native pages intact. */
async function start() {
    const script = document.querySelector('script[data-reference]');
    const response = await fetch(new URL(script.dataset.reference, base));
    if (!response.ok) throw new Error(`Reference HTTP ${response.status}`);
    const data = await response.json();
    for (const field of ['reference', 'renderer']) {
        const expected = document.querySelector(`meta[name="${field}-sha256"]`).content;
        if (data[`${field}_sha256`] !== expected) throw new Error('Stale reference data');
    }
    let current = pageFor(new URL(location.href), data.pages);
    if (!current) return;
    const root = document.querySelector('.layout');
    const footer = root.querySelector('footer').innerHTML;
    const initialBody = root.querySelector('main').innerHTML.trim();
    if (data.pages[current].body.trim() !== initialBody) throw new Error('Reference page mismatch');
    // Clearing only after authentication avoids blank pages when data is unavailable.
    root.replaceChildren();
    history.scrollRestoration = 'manual';

    function show(filename) {
        current = filename;
        const page = data.pages[filename];
        render(h(ReferenceView, { page, filename, menu: data.menu, footer }), root);
        document.title = page.title;
    }

    /** Focus announces page changes; fragments and Back restore useful positions. */
    function position(url, saved) {
        requestAnimationFrame(() => {
            const heading = root.querySelector('h1');
            heading.setAttribute('tabindex', '-1');
            heading.focus({ preventScroll: true });
            let target;
            try { target = document.getElementById(decodeURIComponent(url.hash.slice(1))); }
            catch { /* Invalid fragments should not break navigation. */ }
            if (saved) window.scrollTo(saved.x, saved.y);
            else if (target) target.scrollIntoView();
            else window.scrollTo(0, 0);
        });
    }

    show(current);
    // Keep initial direct-link fragments after replacing the pre-rendered view.
    if (location.hash) position(new URL(location.href));
    document.documentElement.dataset.navigation = 'spa';

    document.addEventListener('click', event => {
        if (event.defaultPrevented || event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
        const link = event.target.closest('a[href]');
        if (!link || link.hasAttribute('download') || (link.target && link.target !== '_self')) return;
        const url = new URL(link.href);
        const filename = pageFor(url, data.pages);
        if (!filename || (filename === current && url.hash)) return;
        event.preventDefault();
        history.replaceState({ ...history.state, referenceScroll: { x: scrollX, y: scrollY } }, '', location.href);
        history.pushState(null, '', url);
        show(filename);
        position(url);
    });

    window.addEventListener('popstate', event => {
        const url = new URL(location.href);
        const filename = pageFor(url, data.pages);
        if (!filename) { location.reload(); return; }
        show(filename);
        position(url, event.state?.referenceScroll);
    });
}

start().catch(error => {
    document.documentElement.dataset.navigation = 'static';
    console.warn('Reference SPA unavailable; using ordinary page links.', error);
});
