import { describe, expect, test } from "bun:test";

import {
  createRootRoute,
  createRoute,
  createRouter,
} from "@tanstack/react-router";

import { landingPath } from "./landing";
import type { LandingFallback } from "./landing";

const routerWith = (
  ...pages: { path: string; landing?: boolean | number }[]
) => {
  const rootRoute = createRootRoute();

  return createRouter({
    routeTree: rootRoute.addChildren(
      pages.map(({ path, landing }) =>
        createRoute({
          getParentRoute: () => rootRoute,
          path,
          staticData: { landing },
        })
      )
    ),
  });
};

/** A `/cases` layout whose index route, at `/cases/`, claims `landing`. */
const casesIndexRouter = () => {
  const rootRoute = createRootRoute();

  const cases = createRoute({
    getParentRoute: () => rootRoute,
    path: "/cases",
  });

  const casesIndex = createRoute({
    getParentRoute: () => cases,
    path: "/",
    staticData: { landing: true },
  });

  return createRouter({
    routeTree: rootRoute.addChildren([cases.addChildren([casesIndex])]),
  });
};

describe("landingPath", () => {
  test("is the route that declares staticData.landing", () => {
    expect(
      landingPath(routerWith({ path: "/a" }, { path: "/b", landing: true }))
    ).toBe("/b");
  });

  test("is the strongest claim when several routes declare it", () => {
    const router = routerWith(
      { path: "/a", landing: true },
      { path: "/b", landing: 10 }
    );

    expect(landingPath(router)).toBe("/b");
  });

  test("goes to the route whose id sorts first between equal claims", () => {
    const router = routerWith(
      { path: "/b", landing: true },
      { path: "/a", landing: true }
    );

    expect(landingPath(router)).toBe("/a");
  });

  test("ignores a NaN claim, which could otherwise never be outranked", () => {
    const router = routerWith(
      { path: "/a", landing: Number.NaN },
      { path: "/b", landing: true }
    );

    expect(landingPath(router)).toBe("/b");
  });

  test("ignores a route that declares it false", () => {
    expect(
      landingPath(routerWith({ path: "/a", landing: false }))
    ).toBeUndefined();
  });

  test("is an index route's URL, not its `/cases/` fullPath the router would redirect away", () => {
    expect(landingPath(casesIndexRouter())).toBe("/cases");
  });

  test("is undefined when no route declares it, so `/` renders its own page", () => {
    expect(landingPath(routerWith({ path: "/a" }))).toBeUndefined();
  });
});

const lastRoute: LandingFallback = (routes) => routes.at(-1);

/** Answers the route at `path`, whether or not it may be landed on. */
const routeAt =
  (path: string): LandingFallback =>
  (routes) =>
    routes.find((candidate) => candidate.fullPath === path);

describe("landingPath's fallbacks", () => {
  test("answer when no route claims it", () => {
    expect(
      landingPath(routerWith({ path: "/a" }, { path: "/b" }), [lastRoute])
    ).toBe("/b");
  });

  test("are not asked when a route claims it", () => {
    const router = routerWith({ path: "/a", landing: true }, { path: "/b" });

    expect(landingPath(router, [lastRoute])).toBe("/a");
  });

  test("are told a route that declares it false may not be picked", () => {
    const router = routerWith({ path: "/a" }, { path: "/b", landing: false });
    const told: boolean[] = [];

    landingPath(router, [
      (routes, canLand) => {
        told.push(
          ...routes.filter((route) => route.fullPath !== "/").map(canLand)
        );

        return routes.find((route) => route.fullPath === "/a");
      },
    ]);

    expect(told).toEqual([true, false]);
  });

  test("cannot pick a route that declares it false, even ignoring canLand", () => {
    const router = routerWith({ path: "/a" }, { path: "/b", landing: false });

    expect(landingPath(router, [routeAt("/b")])).toBeUndefined();
    expect(landingPath(router, [routeAt("/b"), routeAt("/a")])).toBe("/a");
  });

  test("answer in order, an undefined answer passing to the next", () => {
    const router = routerWith({ path: "/a" });

    expect(
      landingPath(router, [routeAt("/missing"), routeAt("/a"), lastRoute])
    ).toBe("/a");
  });
});
