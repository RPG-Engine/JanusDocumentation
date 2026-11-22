/*
 * Janus Engine docs: code blocks.
 *
 * 1. Syntax highlighting. Doxygen only marks keywords, comments, strings, preprocessor lines
 *    and linked symbols. This pass classifies everything else in C++ blocks (functions, types,
 *    namespaces, macros, enum values, members, numbers, operators) by wrapping it in
 *    <span class="tk-*">, without touching Doxygen's links. CMake blocks get a small pass of
 *    their own; other blocks (shell, console output, trees) are left plain.
 *    Colours come from the syntax theme picked in the top bar (theme.css, section 7).
 *
 * 2. API peek. Clicking a linked symbol inside a code block opens its documentation in a side
 *    panel instead of leaving the page. The panel loads the target page in an iframe; pages
 *    shown in that iframe hide the site chrome (html.jd-embedded) and report their URL back,
 *    so "Go to full page" always opens what is currently shown.
 */
(function () {
  'use strict';

  var EMBEDDED = window.self !== window.top;

  function set(words) {
    var s = Object.create(null);
    words.split(/\s+/).forEach(function (w) { if (w) s[w] = true; });
    return s;
  }

  var KEYWORDS = set('alignas alignof asm auto bool char char8_t char16_t char32_t class concept const consteval ' +
    'constexpr constinit const_cast decltype delete double dynamic_cast enum explicit export extern false final float ' +
    'friend inline int long mutable namespace new noexcept nullptr operator override private protected public ' +
    'register reinterpret_cast requires short signed sizeof static static_assert static_cast struct template this ' +
    'thread_local true typedef typeid typename union unsigned using virtual void volatile wchar_t');
  var CONTROL = set('break case catch co_await co_return co_yield continue default do else for goto if return ' +
    'switch throw try while');
  // Types that may appear without a scope.
  var KNOWN_TYPES = set('size_t ptrdiff_t int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t uintptr_t ' +
    'ImVec2 ImVec4 ImU32 ImGuiID ImTextureID ImDrawList ImGuiIO ImGuiStyle ImGuiViewport');
  // Library types recognised only after a scope (std::, glm::, flecs::), since the same words
  // are common variable names ("world", "entity", "query").
  var SCOPED_TYPES = set(
    'string string_view wstring vector deque array map unordered_map set unordered_set list optional variant any ' +
    'function unique_ptr shared_ptr weak_ptr pair tuple span chrono mutex thread atomic exception runtime_error ' +
    'path directory_entry filesystem_error ifstream ofstream fstream stringstream istringstream ostringstream ' +
    'vec2 vec3 vec4 ivec2 ivec3 ivec4 uvec2 uvec3 uvec4 bvec2 dvec2 mat2 mat3 mat4 quat ' +
    'world entity entity_t id query system observer iter type_t string_view ' +
    'steady_clock high_resolution_clock duration time_point mt19937 random_device uniform_real_distribution ' +
    'uniform_int_distribution');

  /* ------------------------------------------------------------------ tokenizer */

  var RE_NUMBER = /^(?:0[xX][0-9a-fA-F']+|0[bB][01']+|(?:\d[\d']*(?:\.[\d']*)?|\.\d[\d']*)(?:[eE][+-]?\d+)?)[uUlLfFzZ]*/;
  var RE_IDENT = /^[A-Za-z_][A-Za-z0-9_]*/;
  var RE_OP = /^(?:->\*?|::|<<=?|>>=?|<=>|[-+*\/%&|^]=?|[=!<>]=?|&&|\|\||\+\+|--|[~?:])/;

  // Splits plain text into tokens: {text, kind} with kind in
  // ws, id, num, str, com, pp, op, punct.
  function tokenize(src, inPreproc) {
    var out = [], i = 0, m, ch, rest;
    var lineStart = true;
    while (i < src.length) {
      rest = src.slice(i);
      ch = src[i];
      if (/\s/.test(ch)) {
        m = /^\s+/.exec(rest); out.push({ text: m[0], kind: 'ws' }); i += m[0].length; continue;
      }
      if (rest.indexOf('//') === 0) { out.push({ text: rest, kind: 'com' }); break; }
      if (rest.indexOf('/*') === 0) {
        var end = rest.indexOf('*/', 2); var len = end < 0 ? rest.length : end + 2;
        out.push({ text: rest.slice(0, len), kind: 'com' }); i += len; continue;
      }
      if (ch === '#' && (lineStart || inPreproc)) {
        m = /^#\s*[A-Za-z_]+/.exec(rest);
        if (m && lineStart) {
          out.push({ text: m[0], kind: 'pp' }); i += m[0].length;
          // #include <file> / "file"
          if (/include/.test(m[0])) {
            var inc = /^(\s*)(<[^>]*>|"[^"]*")/.exec(src.slice(i));
            if (inc) {
              if (inc[1]) out.push({ text: inc[1], kind: 'ws' });
              out.push({ text: inc[2], kind: 'str' }); i += inc[0].length;
            }
          }
          lineStart = false; continue;
        }
        m = /^##?/.exec(rest); out.push({ text: m[0], kind: 'pp' }); i += m[0].length; continue;
      }
      lineStart = false;
      if (ch === '"' || ch === "'") {
        var j = i + 1;
        while (j < src.length && src[j] !== ch) { if (src[j] === '\\') j++; j++; }
        out.push({ text: src.slice(i, j + 1), kind: 'str' }); i = j + 1; continue;
      }
      if ((m = RE_NUMBER.exec(rest)) && !/[A-Za-z_]/.test(src[i - 1] || '')) {
        out.push({ text: m[0], kind: 'num' }); i += m[0].length; continue;
      }
      if ((m = RE_IDENT.exec(rest))) { out.push({ text: m[0], kind: 'id' }); i += m[0].length; continue; }
      if ((m = RE_OP.exec(rest))) { out.push({ text: m[0], kind: m[0] === '::' ? 'punct' : 'op' }); i += m[0].length; continue; }
      out.push({ text: ch, kind: 'punct' }); i += 1;
    }
    return out;
  }

  function isAllCaps(id) { return id.length > 1 && /^_*[A-Z][A-Z0-9_]*$/.test(id) && /[A-Z]{2}/.test(id); }
  function looksLikeType(id) {
    return !!KNOWN_TYPES[id] ||
      /^[FSTEIC][A-Z][a-z]/.test(id) ||          // engine naming: FTransform, STexture2D, TObjectPtr, EType, IEditorTool
      /^SDL_[A-Z][a-z]/.test(id) ||
      /_t$/.test(id);
  }

  // Decides the colour class of an identifier from the code around it.
  function classifyIdent(id, before, after) {
    if (CONTROL[id]) return 'ctl';
    if (KEYWORDS[id]) return 'kw';
    var afterScope = /::\s*$/.test(before);
    var afterMember = /(?:\.|->)\s*$/.test(before);
    var next = after.replace(/^\s+/, '');
    if (/^ImGui[A-Za-z]*_/.test(id)) return 'enum';      // ImGuiCond_Once, ImGuiWindowFlags_NoTitleBar
    if (isAllCaps(id)) return afterScope ? 'enum' : 'macro';
    if (afterScope && SCOPED_TYPES[id]) return 'type';
    if (afterMember) return (next[0] === '(' || next[0] === '<') ? 'fn' : 'member';
    if (next.indexOf('::') === 0) return looksLikeType(id) ? 'type' : 'ns';
    if (next[0] === '(') return looksLikeType(id) ? 'type' : 'fn';
    if (looksLikeType(id)) return 'type';
    if (afterScope) return /^b[A-Z]/.test(id) ? 'member' : (/^[a-z]/.test(id) && next[0] !== '<' ? 'member' : 'type');
    if (next[0] === '<' && /^[A-Z]/.test(id)) return 'type';
    return 'var';
  }

  function span(kind, text) {
    var s = document.createElement('span');
    s.className = 'tk-' + kind;
    s.textContent = text;
    return s;
  }

  // Doxygen spans whose contents are already final.
  var DONE_SELECTOR = 'span.comment, span.stringliteral, span.charliteral, span.keyword, span.keywordtype, ' +
    'span.keywordflow, span.lineno, a, [class^="tk-"]';

  function highlightCppLine(line) {
    var full = line.textContent;
    var walker = document.createTreeWalker(line, NodeFilter.SHOW_TEXT, null);
    var nodes = [], offset = 0, n;
    while ((n = walker.nextNode())) {
      var parent = n.parentElement;
      var skip = parent !== line && parent.closest(DONE_SELECTOR) && line.contains(parent.closest(DONE_SELECTOR));
      nodes.push({ node: n, start: offset, skip: !!skip, pp: !!(parent.closest && parent.closest('span.preprocessor')) });
      offset += n.nodeValue.length;
    }
    nodes.forEach(function (entry) {
      if (entry.skip || !entry.node.nodeValue) return;
      var tokens = tokenize(entry.node.nodeValue, entry.pp);
      var frag = document.createDocumentFragment(), pos = entry.start, changed = false;
      tokens.forEach(function (t) {
        var kind = t.kind;
        if (kind === 'id') kind = classifyIdent(t.text, full.slice(0, pos), full.slice(pos + t.text.length));
        if (kind === 'ws' || kind === 'punct') {
          frag.appendChild(document.createTextNode(t.text));
        } else {
          frag.appendChild(span(kind, t.text)); changed = true;
        }
        pos += t.text.length;
      });
      if (changed) entry.node.parentNode.replaceChild(frag, entry.node);
    });
    // Linked symbols written with their scope ("Log::Flush"): colour the scope as a namespace.
    line.querySelectorAll('a.code').forEach(function (a) {
      if (a.children.length || a.textContent.indexOf('::') < 0) return;
      var parts = a.textContent.split('::');
      var last = parts.pop();
      a.textContent = '';
      parts.forEach(function (p) {
        a.appendChild(span(looksLikeType(p) ? 'type' : 'ns', p));
        a.appendChild(document.createTextNode('::'));
      });
      a.appendChild(document.createTextNode(last));
    });
  }

  var CMAKE_COMMAND = /^\s*([A-Za-z_][A-Za-z0-9_]*)(\s*)\(/;
  function highlightCmakeLine(line) {
    var text = line.textContent;
    var frag = document.createDocumentFragment();
    var i = 0, m;
    var cmd = CMAKE_COMMAND.exec(text);
    if (cmd) {
      var lead = /^\s*/.exec(text)[0];
      if (lead) frag.appendChild(document.createTextNode(lead));
      frag.appendChild(span('fn', cmd[1]));
      i = lead.length + cmd[1].length;
    }
    while (i < text.length) {
      var rest = text.slice(i);
      if (rest[0] === '#') { frag.appendChild(span('com', rest)); break; }
      if (rest[0] === '"') {
        var j = 1; while (j < rest.length && rest[j] !== '"') { if (rest[j] === '\\') j++; j++; }
        frag.appendChild(span('str', rest.slice(0, j + 1))); i += j + 1; continue;
      }
      if ((m = /^\$\{[^}]*\}/.exec(rest))) { frag.appendChild(span('member', m[0])); i += m[0].length; continue; }
      if ((m = /^[A-Z][A-Z0-9_]+\b/.exec(rest))) { frag.appendChild(span('kw', m[0])); i += m[0].length; continue; }
      if ((m = /^\d[\d.]*/.exec(rest))) { frag.appendChild(span('num', m[0])); i += m[0].length; continue; }
      m = /^[^#"$A-Z0-9]+|^./.exec(rest);
      frag.appendChild(document.createTextNode(m[0])); i += m[0].length;
    }
    line.textContent = '';
    line.appendChild(frag);
  }

  function languageOf(fragment) {
    // Visible lines only (Doxygen also puts hidden tooltips inside fragments).
    var lines = Array.prototype.map.call(fragment.querySelectorAll('div.line'), function (l) { return l.textContent; })
      .filter(function (l) { return l.trim(); });
    var text = lines.join('\n');
    // Console/log output: every line starts with "[time]" or "[Logger]" (or is "...").
    if (lines.length && lines.every(function (l) { return /^\s*(\[|\.\.\.)/.test(l); })) return null;
    if (fragment.querySelector('span, a')) return 'cpp';
    if (fragment.closest('div.memdoc')) return 'cpp';                 // macro values, initializers
    if (/^\s*(cmake_minimum_required|project|add_executable|add_subdirectory|add_library|target_[a-z_]+|find_package|set|list|include)\s*\(/m.test(text)) return 'cmake';
    if (/^\s*#include\b|;\s*$|::/m.test(text) && !/^\s*(cmake|git|cd|\.\\|[A-Za-z]:\\)/m.test(text)) return 'cpp';
    return null;
  }

  function highlightAll(root) {
    (root || document).querySelectorAll('div.fragment').forEach(function (fragment) {
      if (fragment.dataset.jdHighlighted) return;
      fragment.dataset.jdHighlighted = '1';
      var lang = languageOf(fragment);
      if (!lang) { fragment.classList.add('jd-plain'); return; }
      fragment.classList.add('jd-lang-' + lang);
      fragment.querySelectorAll('div.line').forEach(lang === 'cmake' ? highlightCmakeLine : highlightCppLine);
    });
  }

  /* ------------------------------------------------------------------ syntax themes */

  var SYNTAX_KEY = 'janus-docs-syntax';
  var SYNTAX_THEMES = ['vs', 'rider', 'vscode', 'dracula'];

  function setSyntax(name) {
    if (SYNTAX_THEMES.indexOf(name) < 0) name = 'vs';
    document.documentElement.setAttribute('data-syntax', name);
    try { localStorage.setItem(SYNTAX_KEY, name); } catch (e) {}
    var select = document.getElementById('jd-syntax-select');
    if (select && select.value !== name) select.value = name;
    syncPeekTheme();
  }

  function currentTheme() {
    return document.documentElement.classList.contains('light-mode') ? 'light' : 'dark';
  }

  /* ------------------------------------------------------------------ API peek panel */

  var peek = null;

  function peekUrl(href) {
    var url = new URL(href, location.href);
    url.searchParams.set('jd-theme', currentTheme());
    url.searchParams.set('jd-syntax', document.documentElement.getAttribute('data-syntax') || 'vs');
    return url;
  }
  function stripPeekParams(href) {
    try {
      var url = new URL(href);
      url.searchParams.delete('jd-theme'); url.searchParams.delete('jd-syntax');
      return url.toString();
    } catch (e) { return href; }
  }

  function buildPeek() {
    var aside = document.createElement('aside');
    aside.className = 'jd-peek';
    aside.setAttribute('aria-label', 'API reference');
    aside.innerHTML =
      '<div class="jd-peek-bar">' +
        '<span class="jd-peek-title">API reference</span>' +
        '<a class="jd-peek-full" href="#">Go to full page' +
          '<svg width="13" height="13" viewBox="0 0 16 16" aria-hidden="true"><path d="M6 3H3v10h10v-3M9 2h5v5M14 2L7 9" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linecap="round" stroke-linejoin="round"/></svg>' +
        '</a>' +
        '<button class="jd-iconbtn jd-peek-close" type="button" aria-label="Close" title="Close (Esc)">' +
          '<svg width="16" height="16" viewBox="0 0 16 16" aria-hidden="true"><path d="M3.5 3.5l9 9M12.5 3.5l-9 9" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/></svg>' +
        '</button>' +
      '</div>' +
      '<iframe class="jd-peek-frame" title="API reference"></iframe>';
    document.body.appendChild(aside);
    peek = {
      root: aside,
      frame: aside.querySelector('iframe'),
      title: aside.querySelector('.jd-peek-title'),
      full: aside.querySelector('.jd-peek-full')
    };
    aside.querySelector('.jd-peek-close').addEventListener('click', closePeek);
    return peek;
  }

  function openPeek(href, label) {
    if (!peek) buildPeek();
    var url = peekUrl(href);
    peek.title.textContent = label || 'API reference';
    peek.opened = stripPeekParams(url.toString());
    peek.full.href = peek.opened;
    peek.frame.src = url.toString();
    document.body.classList.add('jd-peek-open');
  }

  function closePeek() {
    if (!peek) return;
    document.body.classList.remove('jd-peek-open');
  }

  function syncPeekTheme() {
    if (peek && peek.frame.contentWindow) {
      peek.frame.contentWindow.postMessage({
        type: 'jd-theme', theme: currentTheme(),
        syntax: document.documentElement.getAttribute('data-syntax')
      }, '*');
    }
  }

  window.addEventListener('message', function (e) {
    var data = e.data || {};
    if (!EMBEDDED && data.type === 'jd-peek-location' && peek && e.source === peek.frame.contentWindow) {
      var shown = stripPeekParams(data.href);
      peek.full.href = shown;
      // Keep the clicked symbol's name as the title until the reader follows a link in the panel.
      if (data.title && shown !== peek.opened) peek.title.textContent = data.title.trim();
    }
    if (EMBEDDED && data.type === 'jd-theme') {
      var root = document.documentElement;
      root.classList.toggle('dark-mode', data.theme === 'dark');
      root.classList.toggle('light-mode', data.theme === 'light');
      if (data.syntax) root.setAttribute('data-syntax', data.syntax);
    }
  });

  function peekTitleFor(link) {
    var text = link.textContent.replace(/\s+/g, ' ').trim();
    return text || 'API reference';
  }

  document.addEventListener('click', function (e) {
    if (EMBEDDED || e.defaultPrevented || e.button !== 0 || e.ctrlKey || e.metaKey || e.shiftKey || e.altKey) return;
    var link = e.target.closest && e.target.closest('div.fragment a[href], div.memproto a[href]');
    if (!link || link.target === '_blank') return;
    var href = link.getAttribute('href');
    if (!href || /^(https?:|mailto:|#)/.test(href)) return;
    e.preventDefault();
    openPeek(link.href, peekTitleFor(link));
  });

  document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape' && document.body.classList.contains('jd-peek-open')) closePeek();
  });

  /* ------------------------------------------------------------------ start */

  window.JanusCode = { setSyntax: setSyntax, syncPeekTheme: syncPeekTheme, openPeek: openPeek, closePeek: closePeek };

  document.addEventListener('DOMContentLoaded', function () {
    highlightAll();
    var select = document.getElementById('jd-syntax-select');
    if (select) {
      select.value = document.documentElement.getAttribute('data-syntax') || 'vs';
      select.addEventListener('change', function () { setSyntax(select.value); });
    }
    if (EMBEDDED) {
      window.parent.postMessage({
        type: 'jd-peek-location', href: location.href,
        title: (document.querySelector('div.headertitle .title') || {}).textContent || document.title
      }, '*');
    }
  });
})();
