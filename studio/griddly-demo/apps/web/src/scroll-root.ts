/**
 * The page area a layout scrolls instead of the window, such as the design system's shell beside its
 * rail. The router starts it at the top on a navigation, as it does the window, and Back restores it.
 */
export const SCROLL_ROOT_CLASS = "app-scroll-root";

/** The router's scroll handling. `router.tsx` spreads it; a layout's tests can too. */
export const scrollRestorationOptions = {
  scrollRestoration: true,
  scrollToTopSelectors: [`.${SCROLL_ROOT_CLASS}`],
};
