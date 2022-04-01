import Module from "./wasm/jiddly.js";

/**
 * Contains all the code for loading and interfacing with Jiddly
 */
class JiddlyCore {
  constructor() {
    this.moduleOverrides = {
      locateFile: (file, prefix) => {
        if (file === "jiddly.wasm") {
          const newPath = prefix + "../../js/" + file;
          console.log("new path:", newPath);
          return newPath;
        }
      },
    };
  }

  init = async (gdy) => {
    this.module = await new Module(this.moduleOverrides);
    this.jiddly = await new this.module.Jiddly();
    this.gdy = await this.jiddly.loadString(gdy);

    this.game = await this.gdy.createGame("Entity");

    await this.game.registerPlayer("player", "Vector");

    await this.game.init();
    await this.game.reset();

    return this.game.getState();
  };

  getPlayerObservations = () => {};

  getState = async () => {
    return this.game.getState();
  };
}

export default JiddlyCore;
