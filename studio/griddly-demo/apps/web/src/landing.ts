import { resolvePath } from "@tanstack/react-router";
import type {
  AnyRouter,
  StaticDataRouteOption,
  TrailingSlashOption,
} from "@tanstack/react-router";

declare module "@tanstack/react-router" {
  interface StaticDataRouteOption {
    /**
     * Claim the app's landing page: `/` redirects to the strongest claim. `true` is a claim of rank
     * 0; a number is a claim of that rank, for a page that should win over the ordinary ones when
     * both are installed. Equal claims go to the route whose id sorts first. `false` keeps the
     * route from being the landing page at all, even through a fallback.
     */
    landing?: boolean | number;
  }
}

/**
 * What this file reads of a route in the router's `routesById`. Structural on purpose, so a capability
 * that does not import this file can type its own copy.
 */
export interface LandingRoute {
  id: string;
  fullPath: string;
  options: { staticData?: StaticDataRouteOption };
}

/**
 * Picks the route `/` goes to when no route claims it: given every route, in the router's order, and
 * whether a route may be picked (`false` for one that declares `landing: false`). It gets every route
 * rather than the eligible ones so it can order them as it would show them. `landingPath` refuses a
 * route that may not be picked, so `undefined` and a refused route both pass to the next fallback.
 * Another capability adds one as a module under `landing-fallbacks/`, such as a shell sending `/` to
 * its first nav entry.
 */
export type LandingFallback = (
  routes: readonly LandingRoute[],
  canLand: (route: LandingRoute) => boolean
) => LandingRoute | undefined;

const canLand = (route: LandingRoute): boolean =>
  route.options.staticData?.landing !== false;

/**
 * The URL the router serves a route at: its `fullPath` with the trailing slash the router's
 * `trailingSlash` asks for. An index route's `fullPath` is `/cases/`, which the default `"never"`
 * redirects to `/cases`; going there directly saves `/` a second redirect.
 */
const urlOf = (
  // Typed here because `AnyRouter` leaves `trailingSlash` as `any`.
  router: { options: { trailingSlash?: TrailingSlashOption } },
  route: LandingRoute
): string => resolvePath("/", route.fullPath, router.options.trailingSlash);

/**
 * The path `/` redirects to: the route with the strongest `staticData.landing` claim, else the first
 * fallback's answer, if any. Routes are the registry — a capability that wants to be the app's front
 * door says so on its own route, so an app opens on whichever installed feature claims it most
 * strongly.
 */
export const landingPath = (
  router: AnyRouter,
  fallbacks: readonly LandingFallback[] = []
): string | undefined => {
  const routes = Object.values<LandingRoute>(router.routesById);
  let best: { rank: number; route: LandingRoute } | undefined;

  for (const route of routes) {
    const claim = route.options.staticData?.landing;

    if (claim === undefined || claim === false) {
      continue;
    }

    const rank = claim === true ? 0 : claim;

    // `NaN` compares false both ways, so a claim ranked with it could never be displaced.
    if (Number.isNaN(rank)) {
      continue;
    }

    if (
      !best ||
      rank > best.rank ||
      (rank === best.rank && route.id < best.route.id)
    ) {
      best = { rank, route };
    }
  }

  if (best) {
    return urlOf(router, best.route);
  }

  for (const fallback of fallbacks) {
    const route = fallback(routes, canLand);

    if (route !== undefined && canLand(route)) {
      return urlOf(router, route);
    }
  }

  return undefined;
};
