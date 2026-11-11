// The emscripten-generated factory lives at the package root as griddlyjs.js.
// Ambient declarations cannot use relative paths, so match by suffix.
declare module "*griddlyjs.js" {
  import type { GriddlyModuleFactory } from "./module.js";
  const factory: GriddlyModuleFactory;
  export default factory;
}
