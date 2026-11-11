import { afterEach, describe, expect, spyOn, test } from "bun:test";

import {
  RouterProvider,
  createMemoryHistory,
  createRootRoute,
  createRouter,
} from "@tanstack/react-router";
import type { RouteComponent } from "@tanstack/react-router";
import { cleanup, render, screen } from "@testing-library/react";
import type { ReactNode } from "react";

import { NotFound } from "./not-found";
import { RootScreensContext } from "./root-screens";
import type { RootScreens, RootTexts } from "./root-screens";
import { Route as IndexRoute } from "./routes/index";

/**
 * A source that answers in French whatever it is asked, as a capability would contribute; `asked`
 * records the locales it was asked for.
 */
const FRENCH: RootTexts = {
  lang: "fr",
  dir: "ltr",
  missingChunkTitle: "L’application a été mise à jour",
  missingChunk: "Cette page n’a pas pu se charger.",
  reload: "Recharger",
  errorTitle: "Une erreur est survenue",
  errorDescription: "Cette page a rencontré un problème inattendu.",
  retry: "Réessayer",
  errorDetails: "Détails",
  notFound: "Page introuvable",
  notFoundDescription: "Rien ne se trouve à cette adresse.",
  home: "Aller à l’accueil",
  starterEyebrow: "Pour commencer",
  starterHint: ({ file, directory }) => (
    <>
      Modifiez {file} ou ajoutez une route dans {directory}.
    </>
  ),
};

const french = (
  screens: Partial<RootScreens> = {}
): RootScreens & { asked: string[] } => {
  const asked: string[] = [];

  return {
    ...screens,
    asked,
    resolveLocale: () => "fr",
    texts: (locale) => {
      asked.push(locale);

      return FRENCH;
    },
  };
};

const home = (): RouteComponent => {
  const Home = IndexRoute.options.component;

  if (Home === undefined) {
    throw new Error("The index route has no component.");
  }

  return Home;
};

const renderRoot = (
  component: RouteComponent,
  source: RootScreens,
  context?: { locale: string }
) => {
  const router = createRouter({
    routeTree: createRootRoute({
      beforeLoad: () => context,
      component,
      notFoundComponent: NotFound,
    }),
    history: createMemoryHistory(),
  });

  const wrapper = ({ children }: { children: ReactNode }) => (
    <RootScreensContext value={source}>{children}</RootScreensContext>
  );

  render(<RouterProvider router={router} />, { wrapper });
};

afterEach(() => {
  cleanup();
});

describe("the root screens", () => {
  test("the not-found page speaks the provided source in the root route's locale", async () => {
    const source = french();

    renderRoot(NotFound, source, { locale: "fr" });

    await screen.findByText("Page introuvable");
    expect(new Set(source.asked)).toEqual(new Set(["fr"]));
  });

  test("the plain not-found page links home", async () => {
    renderRoot(NotFound, french(), { locale: "fr" });

    const link = await screen.findByRole("link", { name: "Aller à l’accueil" });

    expect(link.getAttribute("href")).toBe("/");
    expect(screen.getByText("Rien ne se trouve à cette adresse.")).toBeTruthy();
  });

  test("the plain starter page renders the source's eyebrow and hint, its paths left to right", async () => {
    renderRoot(home(), french(), { locale: "fr" });

    const file = await screen.findByText("apps/web/src/routes/index.tsx");

    expect(screen.getByText("Pour commencer")).toBeTruthy();
    expect(file.tagName).toBe("CODE");
    expect(file.getAttribute("dir")).toBe("ltr");
    expect(file.parentElement?.textContent).toBe(
      "Modifiez apps/web/src/routes/index.tsx ou ajoutez une route dans apps/web/src/routes/."
    );
  });

  test("with no locale on the root route, they ask for English and never resolve one", async () => {
    const source = french();
    const resolveLocale = spyOn(source, "resolveLocale");

    renderRoot(NotFound, source);

    await screen.findByText("Page introuvable");
    expect(new Set(source.asked)).toEqual(new Set(["en"]));
    expect(resolveLocale).not.toHaveBeenCalled();
  });

  test("a source's own screens replace the plain ones, given the words and the paths", async () => {
    const source = french({
      NotFound: ({ texts }) => <h1>{`404 · ${texts.notFound}`}</h1>,
      Starter: ({ texts, appName, paths }) => (
        <h1>{`${texts.starterEyebrow} · ${appName} · ${paths.file}`}</h1>
      ),
    });

    renderRoot(NotFound, source, { locale: "fr" });
    await screen.findByRole("heading", { name: "404 · Page introuvable" });

    cleanup();
    renderRoot(home(), source, { locale: "fr" });
    // The app's name is the build's (`APP_NAME`, which an app's `.env` may set).
    await screen.findByRole("heading", {
      name: /^Pour commencer · .+ · apps\/web\/src\/routes\/index\.tsx$/u,
    });
  });
});
