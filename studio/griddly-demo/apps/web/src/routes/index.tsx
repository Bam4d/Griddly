import { createFileRoute, Link } from "@tanstack/react-router";

import { useAppName } from "../app-name";

/** Landing page: links into the Griddly WASM demo. */
const Home = () => {
  const appName = useAppName();

  return (
    <main className="mx-auto flex min-h-full max-w-2xl flex-col justify-center gap-4 p-8">
      <p className="text-sm text-neutral-500">Mistral Studio app</p>
      <h1 className="text-3xl font-semibold">{appName}</h1>
      <p className="text-neutral-400">
        A hosted demo of the Griddly game engine, running fully in your browser as WebAssembly via
        the <code>@griddly/wasm</code> npm package (installed from GitHub).
      </p>
      <div>
        <Link
          to="/demo"
          className="inline-block rounded bg-blue-600 px-4 py-2 text-sm font-medium hover:bg-blue-500"
        >
          Open the Griddly demo →
        </Link>
      </div>
    </main>
  );
};

export const Route = createFileRoute("/")({
  component: Home,
});
