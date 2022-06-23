import LZString from "lz-string";

class EditorHistory {
  constructor(maxHistory) {
    this.maxHistory = maxHistory;
  }

  getEnvData(envName) {
    let envDataString = window.localStorage.getItem(envName);

    let envData;
    if (!envDataString) {
      envData = {
        stateHistory: [
          this.compressState({
            gdy: {},
            trajectories: [],
          }),
        ],
      };
    } else {
      envData = JSON.parse(envDataString);
    }

    // TODO: Remove this later
    if ("history" in envData) {
      envData.stateHistory = [];
      envData.history.forEach((gdy) => {
        envData.stateHistory.push(
          this.compressState({
            gdy,
            trajectories: [],
          })
        );
      });

      delete envData["history"];
    }

    this.setLastEnv(envName);

    return envData;
  }

  decompressState(compressedEditorState) {
    const editorState = {};
    for (const key in compressedEditorState) {
      editorState[key] = JSON.parse(
        LZString.decompress(compressedEditorState[key])
      );
    }

    return editorState;
  }

  compressState(editorState) {
    const compressedState = {};
    for (const key in editorState) {
      compressedState[key] = LZString.compress(
        JSON.stringify(editorState[key])
      );
    }
    return compressedState;
  }
    
  getEnvList() {
    return JSON.parse(window.localStorage.getItem("_envList")) || [];
  }

  getLastEnv() {
    return window.localStorage.getItem("_lastEnv");
  }

  setLastEnv(envName) {
    window.localStorage.setItem("_lastEnv", envName);
  }

  updateState(envName, newState) {
    const envData = this.getEnvData(envName);
    const prevState = this.decompressState(
      envData.stateHistory[envData.stateHistory.length - 1]
    );

    const updatedState = {
      ...prevState,
      ...newState,
    };

    envData.stateHistory.push(this.compressState(updatedState));

    if (envData.stateHistory.length >= this.maxHistory) {
      envData.stateHistory.shift();
    }

    this.setLastEnv(envName);

    window.localStorage.setItem(envName, JSON.stringify(envData));
  }

  getState(envName) {
    const envData = this.getEnvData(envName);
    return this.decompressState(
      envData.stateHistory[envData.stateHistory.length - 1]
    );
  }
}

export default EditorHistory;
