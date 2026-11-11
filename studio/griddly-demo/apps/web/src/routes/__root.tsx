import type { QueryClient } from "@tanstack/react-query";
import {
  HeadContent,
  Scripts,
  createRootRouteWithContext,
} from "@tanstack/react-router";
import type { ReactNode } from "react";

import { DEFAULT_APP_NAME } from "../app-name";
import { useRootTexts } from "../root-screens";
import { contributedRootScreens } from "../root-screens-source";

import appCss from "../index.css?url";

export interface RouterContext {
  queryClient: QueryClient;
  /** Where `/` sends the user; see `landing.ts`. */
  landingPath: () => string | undefined;
  /** The app's display name; see `app-name.ts`. Absent only in a test's hand-built router. */
  appName?: string;
}

/**
 * The HTML document. Layout belongs in a layout route, not here: wrap pages in a pathless
 * `routes/_<name>.tsx` so a page can opt out of it by living outside that directory.
 *
 * It renders in the root screens' language and direction (`root-screens.tsx`), so the server's
 * document already says which language it is in. `suppressHydrationWarning` covers attributes a
 * script sets on `<html>` before React hydrates, such as the theme class.
 */
const RootDocument = ({ children }: { children: ReactNode }) => {
  const { lang, dir } = useRootTexts();

  return (
    <html lang={lang} dir={dir} suppressHydrationWarning>
      <head>
        <HeadContent />
      </head>
      <body>
        {children}
        <Scripts />
      </body>
    </html>
  );
};

export const Route = createRootRouteWithContext<RouterContext>()({
  // The locale the root screens speak (`root-screens.tsx`). The server resolves it per request and
  // the browser hydrates with that answer, as the router carries `beforeLoad`'s context; in the
  // browser it re-runs on each navigation and `router.invalidate()`, so a language switch shows.
  beforeLoad: () => ({ locale: contributedRootScreens.resolveLocale() }),
  head: ({ match }) => ({
    meta: [
      { charSet: "utf-8" },
      { name: "viewport", content: "width=device-width, initial-scale=1" },
      { title: match.context.appName ?? DEFAULT_APP_NAME },
    ],
    links: [
      { rel: "stylesheet", href: appCss },
      { rel: "icon", type: "image/svg+xml", href: "/favicon.svg" },
    ],
  }),
  shellComponent: RootDocument,
});
