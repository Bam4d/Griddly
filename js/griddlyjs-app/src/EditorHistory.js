import LZString from "lz-string";

class EditorHistory {
  constructor(maxHistory) {
    this.maxHistory = maxHistory;
  }

  getProjectData(projectName) {
    let projectDataString = window.localStorage.getItem(projectName);

    let projectData;
    if (!projectDataString) {
      projectData = {
        stateHistory: [
          this.compressState({
            gdy: {},
            trajectories: [],
          }),
        ],
      };
    } else {
      projectData = JSON.parse(projectDataString);
    }

    // TODO: Remove this later
    if ("history" in projectData) {
      projectData.stateHistory = [];
      projectData.history.forEach((gdy) => {
        projectData.stateHistory.push(
          this.compressState({
            gdy,
            trajectories: [],
          })
        );
      });

      delete projectData["history"];
    }

    this.setLastProject(projectData);

    return projectData;
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
    
  getProjectNames() {
    if(window.localStorage.getItem("_projectNames")) {
      this.projectNames = new Set(JSON.parse(window.localStorage.getItem("_projectNames")));
    } else {
      this.projectNames = new Set();
    }

    return this.projectNames;
  }

  addProjectName(projectName) {
    this.projectNames.add(projectName);
    window.localStorage.setItem("_projectNames", JSON.stringify(Array.from(this.projectNames)));
  }

  getLastProject() {
    return window.localStorage.getItem("_lastProject");
  }

  setLastProject(projectName) {
    window.localStorage.setItem("_lastProject", projectName);
  }

  updateState(projectName, newState) {
    const projectData = this.getProjectData(projectName);
    const prevState = this.decompressState(
      projectData.stateHistory[projectData.stateHistory.length - 1]
    );

    const updatedState = {
      ...prevState,
      ...newState,
    };

    projectData.stateHistory.push(this.compressState(updatedState));

    if (projectData.stateHistory.length >= this.maxHistory) {
      projectData.stateHistory.shift();
    }

    this.setLastProject(projectName);

    window.localStorage.setItem(projectName, JSON.stringify(projectData));
  }

  getState(projectName) {
    const projectData = this.getProjectData(projectName);
    return this.decompressState(
      projectData.stateHistory[projectData.stateHistory.length - 1]
    );
  }
}

export default EditorHistory;
