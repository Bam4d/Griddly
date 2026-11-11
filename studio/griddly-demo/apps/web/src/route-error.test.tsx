import { beforeEach, describe, expect, spyOn, test } from "bun:test";

import {
  RouterProvider,
  createMemoryHistory,
  createRootRoute,
  createRouter,
} from "@tanstack/react-router";
import { cleanup, fireEvent, render, screen } from "@testing-library/react";
import { Suspense, lazy } from "react";
import type { ComponentType } from "react";

import { RootScreensContext, plainRootScreens } from "./root-screens";
import type { RootScreens } from "./root-screens";
import { RouteError, page } from "./route-error";

// A page whose `React.lazy` panel fails to load, as after a redeploy removed the panel's chunk.
const renderPageFailingWith = (
  fail: () => never,
  source: RootScreens = plainRootScreens
) => {
  const Panel = lazy(async (): Promise<{ default: ComponentType }> => fail());

  const router = createRouter({
    routeTree: createRootRoute({
      // As `routes/__root.tsx` does.
      beforeLoad: () => ({ locale: source.resolveLocale() }),
      component: () => (
        <Suspense>
          <Panel />
        </Suspense>
      ),
    }),
    history: createMemoryHistory(),
    defaultErrorComponent: RouteError,
  });

  render(
    <RootScreensContext value={source}>
      <RouterProvider router={router} />
    </RootScreensContext>
  );
};

const missingChunk = () =>
  new TypeError(
    "Failed to fetch dynamically imported module: http://localhost/assets/panel-3f2a1c.js"
  );

describe("RouteError", () => {
  beforeEach(() => {
    window.sessionStorage.clear();
  });

  test("reloads once for a missing chunk, then offers a Reload button", async () => {
    const reload = spyOn(page, "reload").mockImplementation(() => {
      /* jsdom cannot reload */
    });

    renderPageFailingWith(() => {
      throw missingChunk();
    });
    await screen.findByRole("button", { name: "Reload" });
    expect(reload).toHaveBeenCalledTimes(1);

    // The reloaded page still lacks the chunk: show the button rather than reload again.
    cleanup();
    renderPageFailingWith(() => {
      throw missingChunk();
    });
    await screen.findByRole("button", { name: "Reload" });
    expect(reload).toHaveBeenCalledTimes(1);
  });

  test("shows any other error under a title, its message folded away", async () => {
    const reload = spyOn(page, "reload");

    renderPageFailingWith(() => {
      throw new Error("The API is down");
    });

    await screen.findByText("Something went wrong");
    expect(screen.getByText("Details").tagName).toBe("SUMMARY");
    expect(
      screen.getByText("The API is down").closest("details")
    ).not.toBeNull();
    expect(screen.queryByRole("button", { name: "Reload" })).toBeNull();
    expect(reload).not.toHaveBeenCalled();
  });

  test("shows a thrown value that is not an Error as text", async () => {
    renderPageFailingWith(() => {
      // oxlint-disable-next-line eslint/no-throw-literal, typescript/only-throw-error -- a route can throw anything; this is the case under test
      throw "The API is down";
    });

    await screen.findByText("Something went wrong");
    expect(screen.getByText("The API is down")).toBeTruthy();
  });

  test("speaks the words a contributed source supplies", async () => {
    spyOn(page, "reload").mockImplementation(() => {
      /* jsdom cannot reload */
    });

    renderPageFailingWith(
      () => {
        throw missingChunk();
      },
      {
        resolveLocale: () => "ar",
        texts: () => ({
          ...plainRootScreens.texts("en"),
          lang: "ar",
          dir: "rtl",
          missingChunk: "أعد التحميل للحصول على أحدث إصدار.",
          reload: "إعادة التحميل",
        }),
      }
    );

    await screen.findByRole("button", { name: "إعادة التحميل" });
    expect(screen.getByText("أعد التحميل للحصول على أحدث إصدار.")).toBeTruthy();
  });

  test("Try again runs the page's loader again and shows the page once it succeeds", async () => {
    let calls = 0;

    const router = createRouter({
      routeTree: createRootRoute({
        loader: () => {
          calls += 1;

          if (calls === 1) {
            throw new Error("The API is down");
          }
        },
        component: () => <p>The page</p>,
      }),
      history: createMemoryHistory(),
      defaultErrorComponent: RouteError,
    });

    render(<RouterProvider router={router} />);
    fireEvent.click(await screen.findByRole("button", { name: "Try again" }));

    await screen.findByText("The page");
    expect(calls).toBe(2);
  });

  test("a contributed screen takes the words and the actions, the reload-once logic unchanged", async () => {
    const reload = spyOn(page, "reload").mockImplementation(() => {
      /* jsdom cannot reload */
    });

    renderPageFailingWith(
      () => {
        throw missingChunk();
      },
      {
        ...plainRootScreens,
        MissingChunk: ({ texts, onReload }) => (
          <button type="button" onClick={onReload}>
            {`↻ ${texts.reload}`}
          </button>
        ),
      }
    );

    fireEvent.click(await screen.findByRole("button", { name: "↻ Reload" }));
    // Once on its own, once for the click.
    expect(reload).toHaveBeenCalledTimes(2);
  });
});
