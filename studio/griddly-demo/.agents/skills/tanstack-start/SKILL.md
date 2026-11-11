---
name: tanstack-start
description: Use when adding or changing pages, layouts, data loading or styles in this app's web frontend, `apps/web`, a TanStack Start app with TanStack Router, TanStack Query, Vite and Tailwind CSS.
---

# TanStack Start web app

`apps/web` is a TanStack Start app on React 19. Routes are files, data goes through TanStack Query,
styles are Tailwind CSS 4, and Vite builds it with Nitro serving the result.

## Where things live

| Path | What |
| --- | --- |
| `src/routes/` | One file per route; the file path is the URL. `__root.tsx` renders the HTML document, `index.tsx` is `/`. |
| `src/routeTree.gen.ts` | The route tree the TanStack Start Vite plugin writes. It is gitignored; `bunx nx run web:gen-routes` writes it without a build. |
| `src/router.tsx` | Creates the router, with a fresh `QueryClient` per request on the server and one in the browser. |
| `src/landing.ts` | Lets a route claim `/` with `staticData: { landing: true }`, so `/` redirects to it; `landing: false` keeps a route out of the fallbacks. |
| `src/landing-fallbacks/` | Where `/` goes when no route claims it. Each file default-exports a `LandingFallback`, asked in filename order. |
| `src/app-name.ts` | The app's display name, from `APP_NAME` in the app-root `.env`. |
| `src/index.css` | The Tailwind entry point and global styles. |
| `vite-plugins/` | Vite plugins other capabilities add. Each file default-exports one plugin, or an array of them. |

## Add a page

Create a file under `src/routes/`. `src/routes/reports/index.tsx` serves `/reports` and
`src/routes/reports/$reportId.tsx` serves `/reports/<id>`:

```tsx
import { createFileRoute } from "@tanstack/react-router";

export const Route = createFileRoute("/reports/$reportId")({
  component: Report,
});

function Report() {
  const { reportId } = Route.useParams();
  return <h1 className="text-2xl font-semibold">Report {reportId}</h1>;
}
```

The dev server regenerates the route tree when a route file changes. Link between pages with
`<Link to="/reports/$reportId" params={{ reportId }}>`, which type-checks the path and its params.

Keep layout out of `__root.tsx`. A pathless layout route, `src/routes/_app.tsx` rendering an
`<Outlet />`, wraps every page under `src/routes/_app/` without adding a URL segment.

## Load data

Fetch through TanStack Query, with the query described once by `queryOptions` and read with
`useQuery`. Here `listReports` stands for the call to your API:

```tsx
import { queryOptions, useQuery } from "@tanstack/react-query";

const reportsQuery = queryOptions({
  queryKey: ["reports"],
  queryFn: () => listReports(),
});

function Reports() {
  const { data = [] } = useQuery(reportsQuery);
  return <ul>{data.map((name) => <li key={name}>{name}</li>)}</ul>;
}
```

The router does not carry the query cache from the server render to the browser, so data a route
`loader` prefetches on the server is fetched again in the browser. Keep queries in components unless
you add TanStack Query's SSR integration to `src/router.tsx`. When the app has the FastAPI backend,
call it through the typed client generated into `src/api/`.

## Run and check

`mistral apps dev` from the app root runs the web app with the rest of the app. On its own:

- `bunx nx run web:dev` serves it on port 3001.
- `bunx nx run web:build` builds it, and `bunx nx run web:test` runs its tests with `bun test`.
- Public client settings must start with `VITE_`; everything else stays on the server.
