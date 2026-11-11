// Write src/routeTree.gen.ts without building the app (`bunx nx run web:gen-routes`).
//
// The route tree is gitignored and only the TanStack Start Vite plugin writes it, so a fresh
// checkout cannot type-check until something has run Vite. Creating the dev server (without
// listening) runs the same plugin with the same config as `vite build`, so the tree is identical to
// the one a build writes, in about a second. `quality:check-ts` runs it first when it is present.
import { createServer } from "vite";

const server = await createServer({
  clearScreen: false,
  logLevel: "warn",
  server: { hmr: false, middlewareMode: true, watch: null },
});

await server.close();

// A dev-server plugin keeps the event loop alive after close(); the tree is already written.
process.exit(0);
