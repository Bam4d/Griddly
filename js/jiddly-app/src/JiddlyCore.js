import Module from "./wasm/jiddly.js";

/**
 * Contains all the code for loading and interfacing with Jiddly
 */
class JiddlyCore {
  constructor() {
    const moduleOverrides = {
      locateFile: (file, prefix) => {
        if (file === "jiddly.wasm") {
          const newPath = prefix + "../../js/" + file;
          console.log("new path:", newPath);
          return newPath;
        }
      },
      onRuntimeInitialized: () => {

        console.log("Initialized");

        //const jiddlyInstance = new Module.Jiddly();

        console.log("Instance:");

        // var actionInputMappings = gdy.getActionInputMappings();
        // for (var i = 0; i < actionInputMappings.inputMappings.size(); i++) {
        //   var key = actionInputMappings.inputMappings.keys().get(i);
        //   console.log(actionInputMappings.inputMappings.get(key));
        // }

        //Module.init(testLevelString);
      },
    };

    this.module = Module(moduleOverrides);
  }

  init = async (gdy) => {
    return await this.module.loadString(gdy);
  };

  getPlayerObservations = () => {};

  getState = () => {};
}

export default JiddlyCore;
