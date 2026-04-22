(function () {
  "use strict";

  var LANG_STORAGE_KEY = "hdoc.lang";

  function readStoredLanguage() {
    try {
      if (window.localStorage) {
        var storedLanguage = window.localStorage.getItem(LANG_STORAGE_KEY);
        if (storedLanguage) {
          return storedLanguage;
        }
      }
    }
    catch (err) {
    }

    return null;
  }

  function writeStoredLanguage(lang) {
    try {
      if (window.localStorage && lang) {
        window.localStorage.setItem(LANG_STORAGE_KEY, lang);
      }
    }
    catch (err) {
    }
  }

  function normalizeLanguageTag(lang) {
    if (!lang) {
      return "";
    }

    var parts = String(lang).replace(/_/g, "-").split("-");
    if (!parts[0]) {
      return "";
    }

    parts[0] = parts[0].toLowerCase();
    if (parts[1]) {
      parts[1] = parts[1].toUpperCase();
    }

    return parts.slice(0, 2).join("-");
  }

  function currentLanguageInfo() {
    var wrapper = document.querySelector("[data-current-lang][data-supported-langs]");
    var supported = [];

    if (!wrapper) {
      return null;
    }

    try {
      supported = JSON.parse(wrapper.getAttribute("data-supported-langs") || "[]");
    }
    catch (err) {
      supported = [];
    }

    return {
      current: normalizeLanguageTag(wrapper.getAttribute("data-current-lang")),
      supported: supported.map(normalizeLanguageTag).filter(Boolean)
    };
  }

  function detectBrowserLanguage() {
    var languages = [];

    if (Array.isArray(navigator.languages)) {
      languages = navigator.languages.slice();
    }
    if (navigator.language) {
      languages.push(navigator.language);
    }

    return languages.map(normalizeLanguageTag).filter(Boolean);
  }

  function resolvePreferredLanguage(supported) {
    var requested = detectBrowserLanguage();
    var i;
    var j;
    var requestedPrimary;
    var supportedPrimary;

    for (i = 0; i < requested.length; i++) {
      for (j = 0; j < supported.length; j++) {
        if (supported[j].toLowerCase() === requested[i].toLowerCase()) {
          return supported[j];
        }
      }
    }

    for (i = 0; i < requested.length; i++) {
      requestedPrimary = requested[i].split("-")[0];
      for (j = 0; j < supported.length; j++) {
        supportedPrimary = supported[j].split("-")[0].toLowerCase();
        if (supportedPrimary === requestedPrimary.toLowerCase()) {
          return supported[j];
        }
      }
    }

    return supported.length ? supported[0] : "";
  }

  function maybeApplyInitialLanguagePreference() {
    var info = currentLanguageInfo();
    var switcher = document.querySelector("[data-language-switcher]");
    var storedLanguage;
    var preferredLanguage;
    var option;

    if (!info || !switcher || !info.supported.length) {
      return;
    }

    storedLanguage = normalizeLanguageTag(readStoredLanguage());
    preferredLanguage = storedLanguage && info.supported.indexOf(storedLanguage) !== -1
      ? storedLanguage
      : resolvePreferredLanguage(info.supported);

    if (!preferredLanguage || preferredLanguage === info.current) {
      return;
    }

    option = switcher.querySelector('option[data-lang="' + preferredLanguage + '"]');
    if (option && option.value) {
      window.location.replace(option.value);
    }
  }

  function normalize(value) {
    return (value || "").toString().toLocaleLowerCase();
  }

  function createElement(tag, className, text) {
    var element = document.createElement(tag);
    if (className) {
      element.className = className;
    }
    if (text) {
      element.appendChild(document.createTextNode(text));
    }
    return element;
  }

  function scoreRecord(record, terms) {
    var title = normalize(record.title);
    var text = normalize(record.text);
    var score = 0;

    for (var i = 0; i < terms.length; i++) {
      var term = terms[i];
      if (title === term) {
        score += 20;
      }
      else if (title.indexOf(term) !== -1) {
        score += 12;
      }
      else if (text.indexOf(term) !== -1) {
        score += 4;
      }
      else {
        return 0;
      }
    }

    return score;
  }

  function renderResults(container, form, results) {
    container.innerHTML = "";

    if (results.length === 0) {
      container.appendChild(createElement("div", "doc-search__status", form.getAttribute("data-no-results") || "No results found."));
      container.hidden = false;
      return;
    }

    for (var i = 0; i < results.length; i++) {
      var record = results[i].record;
      var link = createElement("a", "doc-search__result");
      link.href = record.url;
      link.appendChild(createElement("strong", "", record.title));
      link.appendChild(createElement("span", "", record.text));
      container.appendChild(link);
    }

    container.hidden = false;
  }

  function runSearch(input, form, container, index) {
    var query = input.value.replace(/^\s+|\s+$/g, "");
    if (query.length < 2) {
      container.innerHTML = "";
      container.appendChild(createElement("div", "doc-search__status", form.getAttribute("data-empty") || "Search pages and settings"));
      container.hidden = false;
      return;
    }

    var terms = normalize(query).split(/\s+/);
    var results = [];

    for (var i = 0; i < index.length; i++) {
      var score = scoreRecord(index[i], terms);
      if (score > 0) {
        results.push({ score: score, record: index[i] });
      }
    }

    results.sort(function (a, b) {
      if (b.score !== a.score) {
        return b.score - a.score;
      }
      return a.record.title.localeCompare(b.record.title);
    });

    renderResults(container, form, results.slice(0, 12));
  }

  function initSearch() {
    var form = document.querySelector(".doc-search");
    var input = document.getElementById("doc-search-input");
    var container = document.getElementById("doc-search-results");
    var index = window.HDOC_SEARCH_INDEX || [];

    if (!form || !input || !container || index.length === 0) {
      return;
    }

    input.addEventListener("input", function () {
      runSearch(input, form, container, index);
    });

    input.addEventListener("focus", function () {
      if (container.hidden) {
        runSearch(input, form, container, index);
      }
    });

    form.addEventListener("submit", function (event) {
      event.preventDefault();
      runSearch(input, form, container, index);
      var firstResult = container.querySelector(".doc-search__result");
      if (firstResult) {
        window.location.href = firstResult.href;
      }
    });

    document.addEventListener("keydown", function (event) {
      if (event.key === "/" && document.activeElement !== input) {
        event.preventDefault();
        input.focus();
      }
      if (event.key === "Escape") {
        container.hidden = true;
      }
    });

    document.addEventListener("click", function (event) {
      if (!form.contains(event.target)) {
        container.hidden = true;
      }
    });
  }

  function initLanguageSwitcher() {
    var switcher = document.querySelector("[data-language-switcher]");
    if (!switcher) {
      return;
    }

    switcher.addEventListener("change", function () {
      if (switcher.value) {
        var selected = switcher.options[switcher.selectedIndex];
        if (selected && selected.getAttribute("data-lang")) {
          writeStoredLanguage(normalizeLanguageTag(selected.getAttribute("data-lang")));
        }
        window.location.href = switcher.value;
      }
    });
  }

  function initBackToTop() {
    var button = document.querySelector("[data-back-to-top]");
    if (!button) {
      return;
    }

    function updateVisibility() {
      var pageIsLong = document.documentElement.scrollHeight > window.innerHeight * 1.6;
      var scrolledEnough = window.scrollY > Math.max(420, window.innerHeight * 0.75);
      button.classList.toggle("is-visible", pageIsLong && scrolledEnough);
    }

    button.addEventListener("click", function () {
      window.scrollTo({ top: 0, behavior: "smooth" });
    });

    window.addEventListener("scroll", updateVisibility, { passive: true });
    window.addEventListener("resize", updateVisibility);
    updateVisibility();
  }

  function preferredTheme() {
    var theme = "light";

    try {
      if (window.localStorage) {
        var storedTheme = window.localStorage.getItem("hdoc.theme");
        if (storedTheme === "light" || storedTheme === "dark") {
          return storedTheme;
        }
      }
    }
    catch (err) {
    }

    if (window.matchMedia && window.matchMedia("(prefers-color-scheme: dark)").matches) {
      theme = "dark";
    }

    return theme;
  }

  function applyTheme(theme, persist) {
    var normalized = theme === "dark" ? "dark" : "light";

    document.documentElement.setAttribute("data-theme", normalized);
    document.documentElement.style.colorScheme = normalized;

    if (persist !== false) {
      try {
        if (window.localStorage) {
          window.localStorage.setItem("hdoc.theme", normalized);
        }
      }
      catch (err) {
      }
    }

    syncThemeControl();
  }

  function syncThemeControl() {
    var toggle = document.querySelector("[data-theme-toggle]");
    var icon = toggle ? toggle.querySelector(".doc-theme-toggle__icon") : null;
    var theme = document.documentElement.getAttribute("data-theme") === "dark" ? "dark" : "light";
    var label;

    if (!toggle || !icon) {
      return;
    }

    label = theme === "dark"
      ? (toggle.getAttribute("data-theme-light-label") || "Light mode")
      : (toggle.getAttribute("data-theme-dark-label") || "Dark mode");

    toggle.setAttribute("aria-label", label);
    toggle.setAttribute("title", label);
    toggle.setAttribute("data-theme-active", theme);
    icon.innerHTML = theme === "dark"
      ? '<svg viewBox="0 0 24 24" aria-hidden="true" focusable="false"><circle cx="12" cy="12" r="4"></circle><path d="M12 2v2"></path><path d="M12 20v2"></path><path d="m4.93 4.93 1.41 1.41"></path><path d="m17.66 17.66 1.41 1.41"></path><path d="M2 12h2"></path><path d="M20 12h2"></path><path d="m6.34 17.66-1.41 1.41"></path><path d="m19.07 4.93-1.41 1.41"></path></svg>'
      : '<svg viewBox="0 0 24 24" aria-hidden="true" focusable="false"><path d="M12 3a7 7 0 1 0 9 9 9 9 0 1 1-9-9"></path></svg>';
  }

  function initThemeToggle() {
    var toggle = document.querySelector("[data-theme-toggle]");

    if (!toggle) {
      return;
    }

    syncThemeControl();

    toggle.addEventListener("click", function () {
      var current = document.documentElement.getAttribute("data-theme") === "dark" ? "dark" : "light";
      applyTheme(current === "dark" ? "light" : "dark");
    });

    if (window.matchMedia) {
      var mediaQuery = window.matchMedia("(prefers-color-scheme: dark)");
      var handleChange = function () {
        var storedTheme = null;

        try {
          storedTheme = window.localStorage ? window.localStorage.getItem("hdoc.theme") : null;
        }
        catch (err) {
        }

        if (storedTheme !== "light" && storedTheme !== "dark") {
          applyTheme(preferredTheme(), false);
        }
      };

      if (mediaQuery.addEventListener) {
        mediaQuery.addEventListener("change", handleChange);
      }
      else if (mediaQuery.addListener) {
        mediaQuery.addListener(handleChange);
      }
    }
  }

  function initSideTree() {
    var sideTree = document.querySelector(".sidetree");
    var toggles = document.querySelectorAll("[data-tree-toggle]");
    var currentItem = document.querySelector(".sidetree .current");

    if (!sideTree) {
      return;
    }

    function setExpanded(toggle, expanded) {
      var panelId = toggle.getAttribute("aria-controls");
      var panel = panelId ? document.getElementById(panelId) : null;
      var item = toggle.closest("li");

      toggle.setAttribute("aria-expanded", expanded ? "true" : "false");
      if (panel) {
        panel.hidden = !expanded;
      }
      if (item) {
        item.classList.toggle("is-expanded", expanded);
      }
    }

    for (var i = 0; i < toggles.length; i++) {
      (function (toggle) {
        setExpanded(toggle, toggle.getAttribute("aria-expanded") === "true");
        toggle.addEventListener("click", function () {
          var expanded = toggle.getAttribute("aria-expanded") === "true";
          setExpanded(toggle, !expanded);
        });
      }(toggles[i]));
    }

    if (currentItem && sideTree.scrollHeight > sideTree.clientHeight) {
      window.requestAnimationFrame(function () {
        var itemTop = currentItem.offsetTop;
        var itemBottom = itemTop + currentItem.offsetHeight;
        var padding = 24;
        var visibleTop = sideTree.scrollTop + padding;
        var visibleBottom = sideTree.scrollTop + sideTree.clientHeight - padding;

        if (itemTop < visibleTop || itemBottom > visibleBottom) {
          var target = itemTop - Math.max(24, Math.round(sideTree.clientHeight * 0.18));
          sideTree.scrollTop = Math.max(0, target);
        }
      });
    }
  }

  function initMobileMenu() {
    var toggle = document.querySelector("[data-menu-toggle]");
    var sideTree = document.querySelector(".sidetree");
    var backdrop = document.querySelector("[data-menu-backdrop]");

    if (!toggle || !sideTree || !backdrop) {
      return;
    }

    function openMenu() {
      sideTree.classList.add("is-open");
      backdrop.classList.add("is-visible");
      toggle.setAttribute("aria-expanded", "true");
      document.body.style.overflow = "hidden";
    }

    function closeMenu() {
      sideTree.classList.remove("is-open");
      backdrop.classList.remove("is-visible");
      toggle.setAttribute("aria-expanded", "false");
      document.body.style.overflow = "";
    }

    toggle.addEventListener("click", function () {
      var isOpen = sideTree.classList.contains("is-open");
      if (isOpen) {
        closeMenu();
      } else {
        openMenu();
      }
    });

    backdrop.addEventListener("click", closeMenu);

    sideTree.addEventListener("click", function (event) {
      if (event.target.closest("a")) {
        closeMenu();
      }
    });

    document.addEventListener("keydown", function (event) {
      if (event.key === "Escape" && sideTree.classList.contains("is-open")) {
        closeMenu();
        toggle.focus();
      }
    });
  }

  document.addEventListener("DOMContentLoaded", function () {
    maybeApplyInitialLanguagePreference();
    applyTheme(preferredTheme(), false);
    initSearch();
    initLanguageSwitcher();
    initThemeToggle();
    initBackToTop();
    initSideTree();
    initMobileMenu();
  });
}());
