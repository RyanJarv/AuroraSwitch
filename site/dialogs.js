for (const name of ['setup', 'guide']) {
    const dialog = document.getElementById(`${name}-dialog`);
    const button = document.getElementById(`${name}-open`);
    if (typeof dialog.showModal === 'function') {
        document.getElementById(`${name}-fallback`).hidden = true;
        button.hidden = false;
        button.addEventListener('click', () => dialog.showModal());
    }
}
