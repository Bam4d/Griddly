/**
 * Type declarations for the emscripten-generated ES6 module (`griddlyjs.js`)
 * produced from `js/bindings/*.cpp` (embind).
 *
 * The factory is `async (moduleArg?) => Promise<GriddlyModule>` because the
 * build uses `-sMODULARIZE=1 -sEXPORT_ES6=1 -sASYNCIFY`.
 */

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export type EmbindVector<T = any> = {
  size(): number;
  get(index: number): T;
  push_back(value: T): void;
  delete(): void;
};

// eslint-disable-next-line @typescript-eslint/no-explicit-any
export type EmbindMap<K = any, V = any> = {
  size(): number;
  get(key: K): V;
  set(key: K, value: V): void;
  keys(): EmbindVector<K>;
  delete(): void;
};

export type ObservationDescription = {
  Shape: EmbindVector<number>;
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  Variables: EmbindMap<string, any>;
};

export type ObserverType = {
  SPRITE_2D: number;
  BLOCK_2D: number;
  ISOMETRIC: number;
  VECTOR: number;
  ASCII: number;
};

export interface GriddlyJS {
  loadString(gdyString: string): GriddlyJSGDYWrapper;
  getExceptionMessage(excPtr: number): string;
  delete(): void;
}

export interface GriddlyJSGDYWrapper {
  setMaxSteps(maxSteps: number): void;
  getPlayerCount(): number;
  getAvatarObject(): string;
  /** Plain JS array of action names (returned via embind `val`). */
  getExternalActionNames(): string[];
  getLevelCount(): number;
  getObserverType(observerName: string): number;
  getObserverNames(): string[];
  getDefinedObserverNames(): string[];
  /** Plain JS object: action name → { relative, internal, mapToGrid, inputMappings } (embind `val`). */
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  getActionInputMappings(): Record<string, any>;
  createGame(observerName: string): GriddlyJSGameWrapper;
  delete(): void;
}

export interface GriddlyJSGameWrapper {
  getActionTypeId(actionName: string): number;
  init(): void;
  release(): void;
  registerPlayer(name: string, observerName: string): GriddlyJSPlayerWrapper;
  loadLevel(levelId: number): void;
  loadLevelString(levelString: string): void;
  reset(): number;
  getGlobalObservationDescription(): ObservationDescription;
  /** Observation as a plain JS object (embind `val`). */
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  observe(): any;
  observeImage(): Promise<Uint8Array> | Uint8Array;
  stepParallel(playerActions: number[][]): number;
  getWidth(): number;
  getHeight(): number;
  /** Game state as a plain JS object (embind `val`). */
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  getState(): any;
  getGlobalVariableNames(): string[];
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  getObjectVariableMap(objectName: string): Record<string, any>;
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  getGlobalVariables(names: string[]): Record<string, any>;
  getObjectNames(): string[];
  getObjectVariableNames(): string[];
  seedRandomGenerator(seed: number): void;
  delete(): void;
}

export interface GriddlyJSPlayerWrapper {
  getObservationDescription(): ObservationDescription;
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  observe(): any;
  delete(): void;
}

export interface GriddlyModule {
  GriddlyJS: new () => GriddlyJS;
  ObserverType: ObserverType;
  FS: {
    readFile(path: string, opts?: { encoding?: "utf8" | "binary" }): string | Uint8Array;
    readdir(path: string): string[];
    stat(path: string): { mode: number };
    isDir(mode: number): boolean;
    writeFile(path: string, data: string | Uint8Array): void;
    mkdir(path: string): void;
    analyzePath(path: string): { exists: boolean };
  };
  setSDLWindowVisible(visible: boolean): void;
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  [key: string]: any;
}

export type GriddlyModuleFactory = (
  moduleArg?: Partial<GriddlyModule> & { locateFile?: (file: string, prefix: string) => string }
) => Promise<GriddlyModule>;
