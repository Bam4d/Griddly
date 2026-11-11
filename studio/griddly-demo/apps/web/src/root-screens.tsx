import { useRouteContext } from "@tanstack/react-router";
import { createContext, useContext } from "react";
import type { ComponentType, ReactElement } from "react";

/**
 * The words the root route's own screens show: the error pages, the router's fallback not-found
 * page and the starter `/` page, and the language of the HTML document they render in. This
 * capability ships no translations, so they are English unless another capability supplies them
 * (see `root-screens-source.ts`).
 */
export interface RootTexts {
  /** The language the texts are in, for the document's `lang`. */
  readonly lang: string;
  readonly dir: "ltr" | "rtl";
  /** The redeploy page's title. */
  readonly missingChunkTitle: string;
  /** The redeploy page's explanation. */
  readonly missingChunk: string;
  readonly reload: string;
  readonly errorTitle: string;
  readonly errorDescription: string;
  /** The error page's action, which runs the page's loaders again. */
  readonly retry: string;
  /** The summary of the folded-away error message. */
  readonly errorDetails: string;
  readonly notFound: string;
  readonly notFoundDescription: string;
  /** The not-found page's link to `/`. */
  readonly home: string;
  /** The line above the app's name on the starter page. */
  readonly starterEyebrow: string;
  /** The starter page's hint, around the file to edit and the directory to add a route under. */
  readonly starterHint: (paths: {
    readonly file: ReactElement;
    readonly directory: ReactElement;
  }) => ReactElement;
}

/** The page a redeploy leaves behind: its code is gone, so only a reload brings the page back. */
export interface MissingChunkScreenProps {
  readonly texts: RootTexts;
  readonly onReload: () => void;
}

/** Any other error a route throws, with its message, in whatever language it was thrown in. */
export interface ErrorScreenProps {
  readonly texts: RootTexts;
  readonly message: string;
  /** Runs the page's loaders again and clears the error. */
  readonly onRetry: () => void;
}

/** The router's fallback for a URL or a `notFound()` no route handles itself. */
export interface NotFoundScreenProps {
  readonly texts: RootTexts;
}

/** The starter `/` page. */
export interface StarterScreenProps {
  readonly texts: RootTexts;
  readonly appName: string;
  /** The file to edit and the directory to add a route under, which read left to right. */
  readonly paths: { readonly file: string; readonly directory: string };
}

/**
 * Where the root screens' words, and optionally their look, come from. `resolveLocale` reads the
 * reader's locale; `__root.tsx` calls it in its `beforeLoad`, so the server's answer reaches the
 * browser with the page and a `router.invalidate()` (as after a language switch) reads it again.
 * `texts` gives the words in a locale. Plain functions: they run outside React.
 *
 * Each screen this source leaves out renders this capability's plain one. A screen it supplies
 * gets its words and actions as props; what each one does (the reload-once on a redeploy, the
 * locale, the document's `lang` and `dir`) stays here. The screens can render outside any layout
 * route, so a supplied one brings whatever providers its components need.
 */
export interface RootScreens {
  readonly resolveLocale: () => string;
  readonly texts: (locale: string) => RootTexts;
  readonly MissingChunk?: ComponentType<MissingChunkScreenProps>;
  readonly ErrorScreen?: ComponentType<ErrorScreenProps>;
  readonly NotFound?: ComponentType<NotFoundScreenProps>;
  readonly Starter?: ComponentType<StarterScreenProps>;
}

const ENGLISH: RootTexts = {
  lang: "en",
  dir: "ltr",
  missingChunkTitle: "This app was updated",
  missingChunk:
    "This page could not load, likely because the app was updated. Reload to get the latest version.",
  reload: "Reload",
  errorTitle: "Something went wrong",
  errorDescription:
    "This page ran into an unexpected problem. Try again, or come back in a moment.",
  retry: "Try again",
  errorDetails: "Details",
  notFound: "Page not found",
  notFoundDescription: "Nothing lives at this address.",
  home: "Go to the home page",
  starterEyebrow: "Getting started",
  starterHint: ({ file, directory }) => (
    <>
      {/* i18n-exempt: the English fallback; a contributed source translates it */}
      Edit {file}, or add a route under {directory}.
    </>
  ),
};

/** English words and this capability's plain screens. */
export const plainRootScreens: RootScreens = {
  resolveLocale: () => "en",
  texts: () => ENGLISH,
};

/** Which source the root screens read; `router.tsx` provides the contributed one. */
export const RootScreensContext = createContext<RootScreens>(plainRootScreens);

/**
 * The root screens' source, and its words in the locale the root route resolved. A root route that
 * sets none (an app whose `__root.tsx` predates its `beforeLoad`, a test's hand-built router) gets
 * English: the locale is never resolved during render, where the server and the browser could
 * disagree.
 */
export const useRootScreens = () => {
  const screens = useContext(RootScreensContext);

  const locale = useRouteContext({
    from: "__root__",
    // Set by the root route's `beforeLoad`; `undefined` when that threw, or in a hand-built test router.
    select: (context): string | undefined => context.locale,
  });

  return { screens, texts: screens.texts(locale ?? "en") };
};

export const useRootTexts = (): RootTexts => useRootScreens().texts;
