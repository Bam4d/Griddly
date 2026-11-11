import { useRouter } from "@tanstack/react-router";
import type { ErrorComponentProps } from "@tanstack/react-router";
import { useEffect } from "react";

import { useRootScreens } from "./root-screens";
import type { ErrorScreenProps, MissingChunkScreenProps } from "./root-screens";

// What a browser says when a code-split chunk cannot be fetched, in order Chromium, Firefox and
// Safari, then what Vite's preload helper says when a chunk's stylesheet cannot be.
const MISSING_CHUNK_MESSAGES = [
  "Failed to fetch dynamically imported module",
  "error loading dynamically imported module",
  "Importing a module script failed",
  "Unable to preload CSS",
];

/**
 * True for the error a tab left open across a redeploy gets: its build's hashed chunk is no longer
 * served, so a lazy route or a `React.lazy` component cannot load.
 */
const isMissingChunkError = (error: Error): boolean =>
  MISSING_CHUNK_MESSAGES.some((message) => error.message.startsWith(message));

/** Indirection so a test can observe the reload; jsdom cannot reload. */
export const page = {
  reload: () => {
    window.location.reload();
  },
};

/**
 * Reloads unless this chunk already caused a reload in this tab, so a chunk the new build lacks too
 * shows the error instead of reloading forever. The key is the one TanStack Router's
 * `lazyRouteComponent` sets before its own reload: a missing route chunk reloads once in total.
 * Safari's message names no chunk, so there the guard holds for every chunk for the rest of the
 * tab's session, and a later stale chunk offers the button without reloading first. Offline, the
 * same error means no network rather than a new build: the button waits, keeping the page.
 */
const reloadOnce = (error: Error) => {
  const key = `tanstack_router_reload:${error.message}`;

  if (!window.navigator.onLine) {
    return;
  }

  // A browser that blocks storage throws here: without the guard, leave the reload to the button.
  try {
    if (window.sessionStorage.getItem(key) !== null) {
      return;
    }

    window.sessionStorage.setItem(key, "1");
  } catch {
    return;
  }

  page.reload();
};

const PlainMissingChunk = ({ texts, onReload }: MissingChunkScreenProps) => (
  <div className="flex flex-col items-start gap-4 p-8">
    <h1 className="font-semibold">{texts.missingChunkTitle}</h1>
    <p>{texts.missingChunk}</p>
    <button
      type="button"
      className="rounded-md border px-3 py-1.5 text-sm"
      onClick={onReload}
    >
      {texts.reload}
    </button>
  </div>
);

const MissingChunk = ({ error }: { error: Error }) => {
  const { screens, texts } = useRootScreens();
  const Screen = screens.MissingChunk ?? PlainMissingChunk;

  useEffect(() => {
    reloadOnce(error);
  }, [error]);

  return (
    <Screen
      texts={texts}
      onReload={() => {
        page.reload();
      }}
    />
  );
};

const PlainError = ({ texts, message, onRetry }: ErrorScreenProps) => (
  <div className="flex flex-col items-start gap-2 p-8">
    <h1 className="font-semibold">{texts.errorTitle}</h1>
    <p>{texts.errorDescription}</p>
    <button
      type="button"
      className="rounded-md border px-3 py-1.5 text-sm"
      onClick={onRetry}
    >
      {texts.retry}
    </button>
    <details>
      <summary>{texts.errorDetails}</summary>
      {/* The error's own words, in whatever language it was thrown in. */}
      <pre className="text-sm whitespace-pre-wrap" dir="auto">
        {message}
      </pre>
    </details>
  </div>
);

const OtherError = ({ error }: Pick<ErrorComponentProps, "error">) => {
  const { screens, texts } = useRootScreens();
  const router = useRouter();
  const Screen = screens.ErrorScreen ?? PlainError;
  // Typed as an `Error`, but a route can throw anything, a string or a `Response` among others.
  const thrown: unknown = error;
  const message = thrown instanceof Error ? thrown.message : String(thrown);

  return (
    <Screen
      texts={texts}
      message={message}
      // Runs the loaders again and resets the router's error boundaries.
      onRetry={() => {
        void router.invalidate();
      }}
    />
  );
};

/**
 * The router's `defaultErrorComponent`. A page whose code is gone after a redeploy reloads once,
 * then offers a Reload button; any other error shows a title, a Try again button and its message
 * folded away. Each is the contributed screen when there is one (`root-screens.tsx`), else a plain
 * one. A route's own `errorComponent` still takes precedence.
 */
export const RouteError = ({ error }: ErrorComponentProps) =>
  error instanceof Error && isMissingChunkError(error) ? (
    <MissingChunk error={error} />
  ) : (
    <OtherError error={error} />
  );
