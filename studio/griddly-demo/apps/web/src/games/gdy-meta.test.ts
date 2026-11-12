import { describe, expect, test } from "bun:test";

import spidersGdy from "./minigrid-spiders-garden.yaml?raw";
import { normaliseMeta, parseGdy, parseLevels, searchScore } from "./gdy-meta";

describe("gdy-meta", () => {
  test("levels come from the `# Name | blurb` comments, with their map size", () => {
    const levels = parseLevels(spidersGdy);
    expect(levels.length).toBe(6);
    expect(levels[0]).toEqual({
      name: "Garden Gate",
      blurb: "Find the key to open the walled garden.",
      width: 26,
      height: 16,
    });
  });

  test("levels without a comment get a generic name", () => {
    const gdy = "Environment:\n  Levels:\n    - |\n      w w w\n      w A w\n    # Two | second\n    - |\n      w w\nActions: []\n";
    expect(parseLevels(gdy)).toEqual([
      { name: "Level 1", blurb: "", width: 3, height: 2 },
      { name: "Two", blurb: "second", width: 2, height: 1 },
    ]);
  });

  test("cameras are Sprite2D plus the named SPRITE_2D observers", () => {
    const { cameras, avatar } = parseGdy(spidersGdy);
    expect(avatar).toBe("gnome");
    expect(cameras.map((c) => [c.name, c.follows])).toEqual([
      ["Sprite2D", false],
      ["GnomeCam", true],
      ["GnomeCloseUp", true],
    ]);
  });

  test("metadata is normalised when optional fields are missing or bad", () => {
    const m = normaliseMeta({ difficulty: "insane", accent: "red", hud: "x" }, "foo", "Foo Game");
    expect(m).toMatchObject({ id: "foo", name: "Foo Game", difficulty: "medium", accent: "#34d399", hud: [], tags: [] });
    expect(m.controls).toEqual({ move: "move", extra: [] });
  });

  test("search matches names, tags and descriptions, loosely", () => {
    const g = { id: "doggo", name: "Doggo Remastered", tagline: "Fetch sticks", tags: ["maze"], description: "A park" };
    expect(searchScore(g, "")).toBeGreaterThan(0);
    expect(searchScore(g, "dog")).toBeGreaterThan(searchScore(g, "maze"));
    expect(searchScore(g, "park")).toBeGreaterThan(0);
    expect(searchScore(g, "dgrm")).toBeGreaterThan(0);
    expect(searchScore(g, "dog spider")).toBe(0);
  });
});
