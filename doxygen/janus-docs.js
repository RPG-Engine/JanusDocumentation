/*
 * Janus Engine docs: page behaviour.
 *  - builds the sidebar from window.JANUS_NAV (janus-nav.js)
 *  - builds the "On this page" panel with scroll tracking
 *  - theme toggle (dark by default), mobile menu, Ctrl+K to search
 *  - small layout fixes on Doxygen's generated markup
 */
(function () {
  'use strict';

  var THEME_KEY = 'janus-docs-theme';

  function currentFile() {
    var file = location.pathname.split('/').pop();
    return file || 'index.html';
  }

  function el(tag, attrs, children) {
    var node = document.createElement(tag);
    Object.keys(attrs || {}).forEach(function (k) {
      if (k === 'text') node.textContent = attrs[k];
      else if (k === 'html') node.innerHTML = attrs[k];
      else node.setAttribute(k, attrs[k]);
    });
    (children || []).forEach(function (c) { if (c) node.appendChild(c); });
    return node;
  }

  var chevronSvg = '<svg width="14" height="14" viewBox="0 0 24 24" aria-hidden="true"><path d="M9 6l6 6-6 6" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/></svg>';

  /* ---------------- Sidebar ---------------- */

  function isActive(item, file) {
    return item.href && item.href.split('#')[0] === file && !item.pending;
  }

  function buildLink(item, file) {
    var a = el('a', { href: item.href });
    var label = el('span', { 'class': item.code ? 'jd-nav-code' : '', text: item.label });
    a.appendChild(label);
    if (item.pending) {
      a.classList.add('jd-nav-pending');
      a.appendChild(el('span', { 'class': 'jd-nav-pill', text: 'overview' }));
      a.title = 'Not documented yet: opens its summary in the Module overview';
    }
    if (isActive(item, file)) { a.classList.add('jd-active'); a.setAttribute('aria-current', 'page'); }
    return a;
  }

  function buildItem(item, file) {
    var li = el('li');
    if (!item.children) {
      li.appendChild(buildLink(item, file));
      return li;
    }
    var childActive = item.children.some(function (c) { return isActive(c, file); });
    var details = el('details', {});
    if (childActive || isActive(item, file)) details.open = true;
    var summary = el('summary');
    summary.appendChild(buildLink(item, file));
    var chev = el('span', { 'class': 'jd-nav-chevron', html: chevronSvg, role: 'button', 'aria-label': 'Expand ' + item.label });
    summary.appendChild(chev);
    // Clicking the label navigates; clicking the chevron toggles.
    summary.addEventListener('click', function (e) {
      if (!e.target.closest('.jd-nav-chevron')) { e.preventDefault(); location.href = item.href; return; }
      e.preventDefault();
      details.open = !details.open;
    });
    details.appendChild(summary);
    var ul = el('ul');
    item.children.forEach(function (c) { ul.appendChild(buildItem(c, file)); });
    details.appendChild(ul);
    li.appendChild(details);
    return li;
  }

  function buildSidebar() {
    var host = document.getElementById('jd-sidebar');
    if (!host || !window.JANUS_NAV) return;
    var file = currentFile();
    var nav = el('nav', { 'class': 'jd-nav' });
    window.JANUS_NAV.forEach(function (section) {
      var sec = el('div', { 'class': 'jd-nav-section' });
      sec.appendChild(el('p', { 'class': 'jd-nav-title', text: section.title }));
      var ul = el('ul');
      section.items.forEach(function (item) { ul.appendChild(buildItem(item, file)); });
      sec.appendChild(ul);
      nav.appendChild(sec);
    });
    host.appendChild(nav);
    var active = host.querySelector('.jd-active');
    if (active && active.scrollIntoView) active.scrollIntoView({ block: 'center' });
  }

  /* ---------------- On this page ---------------- */

  function headingId(h) {
    if (h.id) return h.id;
    var inner = h.querySelector('a[id], a[name]');
    if (inner) return inner.id || inner.getAttribute('name');
    var prev = h.previousElementSibling;
    if (prev && prev.tagName === 'A' && (prev.id || prev.name)) return prev.id || prev.name;
    h.id = 'jd-h-' + Math.random().toString(36).slice(2, 9);
    return h.id;
  }

  function headingText(h) {
    var clone = h.cloneNode(true);
    clone.querySelectorAll('.permalink, .mlabels, .anchor').forEach(function (n) { n.remove(); });
    return clone.textContent.replace(/^[\s◆]+/, '').replace(/\s+/g, ' ').trim();
  }

  function buildToc() {
    var host = document.getElementById('jd-toc');
    var contents = document.querySelector('div.contents');
    if (!host || !contents) return;
    var nodes = contents.querySelectorAll('h1.doxsection, h2.doxsection, h3.doxsection, h2.groupheader, h2.memtitle');
    var items = [];
    nodes.forEach(function (h) {
      if (h.closest('details')) return;
      var level = 1;
      if (h.matches('h2.doxsection, h2.memtitle')) level = 2;
      if (h.matches('h3.doxsection')) level = 3;
      items.push({ h: h, level: level, id: headingId(h), text: headingText(h), code: h.matches('h2.memtitle') });
    });
    if (items.length < 2) { document.body.classList.add('jd-no-toc'); return; }

    host.appendChild(el('p', { 'class': 'jd-toc-title', text: 'On this page' }));
    var ul = el('ul');
    var links = items.map(function (it) {
      var a = el('a', { href: '#' + it.id, text: it.text, 'class': 'jd-toc-l' + it.level + (it.code ? ' jd-toc-code' : '') });
      ul.appendChild(el('li', {}, [a]));
      return a;
    });
    host.appendChild(ul);

    var ticking = false;
    function update() {
      ticking = false;
      var current = 0;
      for (var i = 0; i < items.length; i++) {
        if (items[i].h.getBoundingClientRect().top < 110) current = i; else break;
      }
      links.forEach(function (a, i) { a.classList.toggle('jd-active', i === current); });
      var act = links[current];
      if (act && host.scrollHeight > host.clientHeight) {
        var r = act.getBoundingClientRect(), hr = host.getBoundingClientRect();
        if (r.top < hr.top || r.bottom > hr.bottom) host.scrollTop += r.top - hr.top - hr.height / 2;
      }
    }
    window.addEventListener('scroll', function () { if (!ticking) { ticking = true; requestAnimationFrame(update); } }, { passive: true });
    update();
  }

  /* ---------------- Doxygen markup fixes ---------------- */

  function hideExcludedRows() {
    // Classes listed in EXCLUDE_SYMBOLS have no page, but Doxygen still lists them (unlinked,
    // in bold) in their namespace's summary table. Remove those rows.
    document.querySelectorAll('table.memberdecls tr[class^="memitem:"]').forEach(function (row) {
      var right = row.querySelector('td.memItemRight');
      if (right && right.children.length === 1 && right.firstElementChild.tagName === 'B' &&
          right.textContent.trim() === right.firstElementChild.textContent.trim()) {
        var next = row.nextElementSibling;
        if (next && next.className && next.className.indexOf('memdesc:') === 0) next.remove();
        row.remove();
      }
    });
  }

  // Doxygen links plain words such as "ECS" or "Input" to their namespace pages. In prose,
  // the module page is the better target, so send those links there instead.
  var MODULE_PAGES = {
    'namespace_engine.html': 'group__engine__core.html',
    'namespace_log.html': 'group__logging.html',
    'namespace_filesystem.html': 'group__filesystem.html',
    'namespace_memory.html': 'group__memory.html',
    'namespace_e_c_s.html': 'group__ecs.html',
    'namespace_renderer.html': 'group__rendering.html',
    'namespace_input.html': 'group__input.html',
    'namespace_core_types.html': 'group__coretypes.html',
    'namespace_graphics.html': 'group__coretypes.html',
    'namespace_world.html': 'group__coretypes.html',
    'namespace_gameplay_framework.html': 'group__gameplay.html',
    'namespace_im_gui_wrapper.html': 'group__imguiwrapper.html',
    'namespace_core_editor.html': 'group__editor.html'
  };
  function linkWordsToModules() {
    document.querySelectorAll('div.contents div.textblock a.el, div.contents div.memdoc a.el').forEach(function (a) {
      var target = MODULE_PAGES[(a.getAttribute('href') || '').split('#')[0]];
      if (target && a.textContent.indexOf('::') === -1) a.setAttribute('href', target);
    });
  }

  function tidyHeader() {
    // Show "Library » Class" breadcrumbs above the title, like a manual.
    var title = document.querySelector('div.headertitle .title');
    var groups = title && title.querySelector('.ingroups');
    if (groups) {
      groups.classList.add('jd-breadcrumb');
      title.parentNode.insertBefore(groups, title);
    }
  }

  function addAiNotice() {
    // Pages that show code written for these docs (\genex blocks) get one notice at the top.
    var contents = document.querySelector('div.contents');
    if (!contents || !contents.querySelector('details.jd-generated')) return;
    var notice = el('div', { 'class': 'jd-ai-notice', role: 'note' }, [
      el('strong', null, [document.createTextNode('AI-generated content. ')]),
      document.createTextNode('The example code on this page was written by AI for this documentation. ' +
        'Every example was compiled and run against the documented engine version, but it may still contain mistakes. ' +
        'Blocks marked "Engine example" are taken from the engine itself. ')
    ]);
    var more = el('a', { href: 'conventions.html' }, [document.createTextNode('About these examples')]);
    notice.appendChild(more);
    contents.insertBefore(notice, contents.firstChild);
  }

  /* ---------------- Theme / menu / search ---------------- */

  function setTheme(theme) {
    var root = document.documentElement;
    root.classList.toggle('dark-mode', theme === 'dark');
    root.classList.toggle('light-mode', theme === 'light');
    try { localStorage.setItem(THEME_KEY, theme); } catch (e) {}
    if (window.JanusCode) window.JanusCode.syncPeekTheme();
  }

  window.JanusDocs = {
    toggleTheme: function () {
      setTheme(document.documentElement.classList.contains('dark-mode') ? 'light' : 'dark');
    },
    toggleMenu: function (force) {
      var open = typeof force === 'boolean' ? force : !document.body.classList.contains('jd-menu-open');
      document.body.classList.toggle('jd-menu-open', open);
    }
  };

  document.addEventListener('keydown', function (e) {
    if ((e.ctrlKey || e.metaKey) && (e.key === 'k' || e.key === 'K')) {
      var field = document.getElementById('MSearchField');
      if (field) { e.preventDefault(); field.focus(); field.select(); }
    }
  });

  function initSearch() {
    // Doxygen normally wires up its search box from its own menu/tree view, which this theme
    // replaces. Without its config element search.js does nothing, so start it here.
    if (!document.getElementById('doxygen-config') && typeof window.init_search === 'function' &&
        document.getElementById('MSearchField')) {
      try { window.init_search(); } catch (e) { console.warn('Search init failed', e); }
    }
  }

  document.addEventListener('DOMContentLoaded', function () {
    buildSidebar();
    hideExcludedRows();
    linkWordsToModules();
    tidyHeader();
    addAiNotice();
    buildToc();
    initSearch();
  });
})();
