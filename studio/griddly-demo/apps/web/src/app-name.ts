import { useRouteContext } from "@tanstack/react-router";
import { createIsomorphicFn } from "@tanstack/react-start";

/** Shown when nothing names the app (a bare shell, or a test run). */
export const DEFAULT_APP_NAME = "App";

/**
 * The name the bundle was built with: `vite.config.ts` defines it from `APP_NAME` in the app's
 * root `.env` (the variable `mistral apps init` writes), or from the web image's build arg.
 */
const BUILT_APP_NAME =
  import.meta.env.APP_NAME === undefined || import.meta.env.APP_NAME === ""
    ? DEFAULT_APP_NAME
    : import.meta.env.APP_NAME;

/**
 * The app's display name, resolved where the page is rendered: the server's `APP_NAME` when the
 * deployment sets one at runtime, else the build-time name. The router carries the server's answer
 * to the browser (`router.tsx`), so the title and brand never disagree across hydration.
 */
export const resolveAppName = createIsomorphicFn()
  .server(() =>
    process.env.APP_NAME === undefined || process.env.APP_NAME === ""
      ? BUILT_APP_NAME
      : process.env.APP_NAME
  )
  .client(() => BUILT_APP_NAME);

/** The document title and the sidebar brand. Rename the app through `APP_NAME`, not here. */
export const useAppName = (): string =>
  useRouteContext({
    from: "__root__",
    select: (context) => context.appName,
  }) ?? BUILT_APP_NAME;
