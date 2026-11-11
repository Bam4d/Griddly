import { afterEach, mock } from "bun:test";

import { JSDOM } from "jsdom";

// `pretendToBeVisual` supplies requestAnimationFrame. Without it framer-motion cannot tick, so
// an `AnimatePresence` exit never completes and the element it is retiring stays in the DOM
// forever — a test asserting something has gone away can never pass.
// The app has no jsdom typings, so type the instance as the browser globals it provides.
/** @type {{ window: Window & typeof globalThis }} */
// oxlint-disable-next-line typescript/no-unsafe-assignment, typescript/no-unsafe-call -- jsdom ships no types (no @types/jsdom); the JSDoc type above restores them
const dom = new JSDOM("<!doctype html><html><body></body></html>", {
  url: "http://localhost/",
  pretendToBeVisual: true,
});

const { window } = dom;

const globals = {
  window,
  document: window.document,
  navigator: window.navigator,
  self: window,
  Node: window.Node,
  Element: window.Element,
  HTMLElement: window.HTMLElement,
  // framer-motion checks for it once `requestAnimationFrame` (below) lets its animations run.
  SVGElement: window.SVGElement,
  HTMLFormElement: window.HTMLFormElement,
  Event: window.Event,
  CustomEvent: window.CustomEvent,
  FocusEvent: window.FocusEvent,
  KeyboardEvent: window.KeyboardEvent,
  MouseEvent: window.MouseEvent,
  MutationObserver: window.MutationObserver,
  // react-aria builds its collections in a fragment.
  DocumentFragment: window.DocumentFragment,
  HTMLAnchorElement: window.HTMLAnchorElement,
  // Radix's focus scope walks the DOM with a tree walker.
  NodeFilter: window.NodeFilter,
  HTMLInputElement: window.HTMLInputElement,
  HTMLTextAreaElement: window.HTMLTextAreaElement,
  getComputedStyle: window.getComputedStyle.bind(window),
  // The router's scroll restoration sets `history.scrollRestoration`.
  history: window.history,
  // jsdom lays nothing out and does not implement scrolling; the router restores scroll on navigation.
  scrollTo: () => {
    /* no-op: jsdom has no layout to scroll */
  },
};

for (const [name, value] of Object.entries(globals)) {
  Object.defineProperty(globalThis, name, {
    configurable: true,
    value,
    writable: true,
  });
}

// `AbortController` is Bun's, not jsdom's, so a listener registered on a jsdom node with
// `{ signal }` (react-resizable-panels does, for its resize handles) fails jsdom's type check.
// Bridge the signal instead: a jsdom signal that aborts when Bun's does.
// oxlint-disable-next-line typescript/unbound-method -- always invoked with `.call(this, …)` below
const { addEventListener: nativeAddEventListener } =
  window.EventTarget.prototype;

/**
 * @this {EventTarget}
 * @param {string} type - The event type.
 * @param {EventListenerOrEventListenerObject | null} listener - The listener to register.
 * @param {boolean | AddEventListenerOptions} [options] - Capture flag or listener options.
 */
window.EventTarget.prototype.addEventListener = function addEventListener(
  type,
  listener,
  options
) {
  // `options` is a boolean (capture) or an options object; only the object form carries a signal.
  // Typed structurally: it may be Bun's signal, which is not a jsdom `AbortSignal`.
  /** @type {Pick<AbortSignal, "aborted" | "addEventListener"> | undefined} */
  const signal = options instanceof Object ? options.signal : undefined;

  if (signal === undefined || signal instanceof window.AbortSignal) {
    nativeAddEventListener.call(this, type, listener, options);

    return;
  }

  const bridged = new window.AbortController();

  if (signal.aborted) {
    bridged.abort();
  } else {
    signal.addEventListener(
      "abort",
      () => {
        bridged.abort();
      },
      { once: true }
    );
  }

  nativeAddEventListener.call(this, type, listener, {
    ...options,
    signal: bridged.signal,
  });
};

// jsdom lacks the pointer-capture and scrolling methods Radix's Select calls when it opens.
// oxlint-disable-next-line typescript/unbound-method -- only checks whether jsdom defines it
window.Element.prototype.hasPointerCapture ??= () => false;

// oxlint-disable-next-line typescript/unbound-method -- only checks whether jsdom defines it
window.Element.prototype.setPointerCapture ??= () => {
  /* no-op: jsdom has no pointer capture or layout */
};

// oxlint-disable-next-line typescript/unbound-method -- only checks whether jsdom defines it
window.Element.prototype.releasePointerCapture ??= () => {
  /* no-op: jsdom has no pointer capture or layout */
};

// oxlint-disable-next-line typescript/unbound-method -- only checks whether jsdom defines it
window.Element.prototype.scrollIntoView ??= () => {
  /* no-op: jsdom has no pointer capture or layout */
};

/**
 * jsdom keeps `scrollTop` but has no `Element.scrollTo`, which the router calls to start a page area
 * at the top on navigation.
 * @this {Element}
 * @param {[options?: ScrollToOptions] | [x: number, y: number]} args Where to scroll, as an options
 *   object (an omitted axis stays put) or as `x, y`, the two forms the browser accepts.
 */
const scrollElementTo = function scrollElementTo(...args) {
  const options = args.length === 2 ? { left: args[0], top: args[1] } : args[0];

  this.scrollTop = options?.top ?? this.scrollTop;
  this.scrollLeft = options?.left ?? this.scrollLeft;
};

// oxlint-disable-next-line typescript/unbound-method -- only checks whether jsdom defines it
window.Element.prototype.scrollTo ??= scrollElementTo;

/**
 * jsdom has no `matchMedia`; the design system's layouts read their breakpoints through it. This one
 * answers as a screen wider than every breakpoint: a `min-width` (or `width >`, `width >=`) query
 * matches, any other (`max-width`, `prefers-reduced-motion`, …) does not.
 * @param {string} media The media query.
 * @returns {MediaQueryList} A list that never changes, so its listeners are never called.
 */
const matchMedia = (media) =>
  Object.assign(new EventTarget(), {
    matches: /min-width|width\s*>/u.test(media),
    media,
    onchange: null,
    addListener: () => {
      /* no-op: the stand-in screen never resizes */
    },
    removeListener: () => {
      /* no-op: the stand-in screen never resizes */
    },
  });

// On the window's prototype, not the window: a test that stands in its own `matchMedia` and then
// deletes it uncovers this one rather than leaving the next file without any.
if (!("matchMedia" in window)) {
  Object.defineProperty(Object.getPrototypeOf(window), "matchMedia", {
    configurable: true,
    value: matchMedia,
    writable: true,
  });
}

// jsdom lays nothing out, so nothing resizes: charts and dialogs observe their size through this.
globalThis.ResizeObserver ??= class ResizeObserver {
  // oxlint-disable-next-line class-methods-use-this -- nothing to observe
  observe() {
    /* no-op: jsdom has no layout */
  }

  // oxlint-disable-next-line class-methods-use-this -- nothing to observe
  unobserve() {
    /* no-op: jsdom has no layout */
  }

  // oxlint-disable-next-line class-methods-use-this -- nothing to observe
  disconnect() {
    /* no-op: jsdom has no layout */
  }
};

Object.defineProperty(globalThis, "PointerEvent", {
  configurable: true,
  writable: true,
  value: window.PointerEvent ?? window.MouseEvent,
});

globalThis.IS_REACT_ACT_ENVIRONMENT = true;

// Radix's tabs schedule their indicator with `requestAnimationFrame`, which jsdom under bun lacks.
// oxlint-disable-next-line promise/prefer-await-to-callbacks -- a polyfill of the callback-based browser API
globalThis.requestAnimationFrame ??= (callback) =>
  setTimeout(() => {
    // oxlint-disable-next-line promise/prefer-await-to-callbacks -- same polyfill: the browser API calls back
    callback(Date.now());
  }, 0);

globalThis.cancelAnimationFrame ??= (handle) => {
  clearTimeout(handle);
};

// Imported only now: react-dom decides whether it can use the DOM when it first loads.
const { cleanup } = await import("@testing-library/react");

// Bun runs every test file in one process, so a root left mounted or a `spyOn(globalThis, "fetch")`
// left in place leaks into the next file, which passes alone and fails in the full run. Spies are
// therefore torn down after each test: create them in the test or `beforeEach`, never in `beforeAll`.
// A global replaced by assignment (`globalThis.fetch = …`) is not a spy: restore it yourself.
// oxlint-disable-next-line typescript/no-unsafe-call -- the tsconfig program covers .ts/.tsx only, so `bun:test` is untyped here
afterEach(() => {
  cleanup();
  // oxlint-disable-next-line typescript/no-unsafe-call, typescript/no-unsafe-member-access -- same: `bun:test` is untyped in .js
  mock.restore();
});
