import { createFileRoute, redirect } from "@tanstack/react-router";

/** The original Spiders Garden demo now lives in the games library: `/demo` → `/play/spiders`. */
export const Route = createFileRoute("/demo")({
  beforeLoad: () => {
    throw redirect({ to: "/play/$gameId", params: { gameId: "spiders" }, replace: true });
  },
});
