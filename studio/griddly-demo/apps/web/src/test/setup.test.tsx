import { describe, expect, spyOn, test } from "bun:test";

import { render } from "@testing-library/react";

// The two tests run in order: the first leaves a stub and a mounted tree behind, and the second
// fails unless the preload's `afterEach` tore both down, as it must between test files.
describe("test setup", () => {
  const realFetch = globalThis.fetch;

  test("a test may stub fetch and leave a tree mounted", () => {
    spyOn(globalThis, "fetch").mockResolvedValue(new Response("stub"));
    render(<p>left mounted</p>);

    expect(globalThis.fetch).not.toBe(realFetch);
  });

  test("the next test sees the real fetch and an empty document", () => {
    expect(globalThis.fetch).toBe(realFetch);
    expect(document.body.innerHTML).toBe("");
  });
});
