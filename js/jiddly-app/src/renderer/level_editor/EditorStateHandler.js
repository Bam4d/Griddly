class EditorStateHandler {
  constructor(gdy) {
    this.defaultTileSize = 24;
    this.objectTemplates = {};

    this.characterToObject = {};

    this.gdy = gdy;

    this.editorHistory = [];

    this.loadObjects(gdy);

    this.initialState = {
      objects: {},
    };

    this.pushState(this.initialState);
  }

  getObjectLocationKey = (x, y) => {
    return `${x},${y}`;
  };

  getDirection(directionChar) {
    switch(directionChar) {
      case "U":
        return "UP"
      case "D":
        return "DOWN"
      case "L":
        return "LEFT"
      case "R":
        return "RIGHT"
    }
  }

  loadObjects(gdy) {
    this.characterToObject["."] = "background";

    gdy.Objects.forEach((object) => {
      this.characterToObject[object.MapCharacter] = object.Name;
    });
  }

  loadLevelString(levelString) {
    console.log("loading level string", levelString);

    const rotationRegex = /\[(?<directionChar>.*)\]$/;
    const playerIdRegex = /(?<playerId>\d+)/;

    const levelRows = levelString.split("\n");
    for (const row in levelRows) {
      const levelRow = levelRows[row].trim();

      const rowObjects = levelRow.split(/\s+/);
      for (const col in rowObjects) {
        const objectCharsList = rowObjects[col].split("/");

        for (const obj in objectCharsList) {
          const objectChars = objectCharsList[obj];

          const objectName = this.characterToObject[objectChars[0]];
          let directionChar = "U";
          let playerId = 0;

          if (rotationRegex.test()) {
            const result = rotationRegex.exec(objectChars);
            directionChar = result.groups.directionChar;
          }

          if (playerIdRegex.test()) {
            const result = playerIdRegex.exec(objectChars);
            playerId = result.groups.playerId;
          }

          this.addTile(col, row, objectName, playerId, directionChar);
        }
      }
    }
  }

  pushState = (state) => {
    // Copy the state and add it to the history
    const stateCopy = { ...state };
    this.editorHistory.push(stateCopy);

    const historyLength = this.editorHistory.length;

    if (historyLength >= 20) {
      this.editorHistory.pop();
    }
  };

  addTile(x, y, objectName, playerId, directionChar) {
    const state = this.getState();

    const direction = this.getDirection(directionChar)

    const objectInfo = {
      id: objectName+"0",
      name: objectName,
      playerId,
      direction,
      location: {x, y}
    }

    state.objects[this.getObjectLocationKey(x,y)] = objectInfo;

    this.pushState(state);
  }

  removeTile(x, y) {
    const state = this.getState();

    const locationKey = this.getObjectLocationKey(x, y);

    const tileData = state.objects[locationKey];
    state.tileTypeCount[tileData.category]--;
    delete state.objects[locationKey];

    this.pushState(state);
  }

  getState() {
    return { ...this.editorHistory[0] };
  }

  toLevelString() {}
}

export default EditorStateHandler;
