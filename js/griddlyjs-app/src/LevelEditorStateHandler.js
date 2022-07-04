import { hashString, getRandomHash } from "./Utils";

const MR_READ_NORMAL = 0;
const MR_READ_PLAYERID = 1;
const MR_READ_INITIAL_ORIENTATION = 2;

class EditorStateHandler {
  constructor() {
    this.defaultTileSize = 24;
    this.minimumObjectChars = 3;

    this.minimumStateWidth = 3;
    this.minimumStateHeight = 3;
  }

  loadGDY(gdy) {
    this.characterToObject = {};
    this.objectToCharacterAndZ = {};

    this.gdy = gdy;
    this.characterToObject["."] = "background";
    this.objectToCharacterAndZ["background"] = { char: ".", z: 10 };

    this.gdy.Objects.forEach((object) => {
      this.characterToObject[object.MapCharacter] = object.Name;
      this.objectToCharacterAndZ[object.Name] = {
        char: object.MapCharacter,
        z: object.Z || 0,
      };
    });
  }

  getObjectLocationKey = (x, y) => {
    return `${x},${y}`;
  };

  resetEditorState(levelId) {
    this.editorHistory = [];

    this.initialState = {
      objects: {},
      gridWidth: 0,
      gridHeight: 0,
      minx: Number.MAX_VALUE,
      miny: Number.MAX_VALUE,
      maxx: Number.MIN_VALUE,
      maxy: Number.MIN_VALUE,
      tileTypeCount: {},
      levelId: levelId
    };

    // Just used to create a random id
    this.objectId = 0;

    // Another simple way to create a unique id for consecutive states so we dont always re-draw unecessarily
    this.hash = 0;

    this.pushState(this.initialState);
  }

  loadLevelString(levelString, levelId) {
    this.resetEditorState(levelId);
    let mapReaderState = MR_READ_NORMAL;

    const levelStringLength = levelString.length;

    let rowCount = 0;
    let colCount = 0;
    let firstColCount = 0;

    let charIdx = 0;

    let currentObjectName = "";

    let currentPlayerIdChars = [];
    let currentDirection = "NONE";

    let prevChar = "";

    while (charIdx < levelStringLength) {
      const ch = levelString[charIdx];

      switch (ch) {
        case "\n":
          if (mapReaderState === MR_READ_PLAYERID) {
            this.addTile(
              colCount,
              rowCount,
              currentObjectName,
              Number(currentPlayerIdChars.join()),
              currentDirection,
              false
            );
            currentDirection = "NONE";
            mapReaderState = MR_READ_NORMAL;
            currentPlayerIdChars = [];
            colCount++;
          }

          if (rowCount === 0) {
            firstColCount = colCount;
          } else if (firstColCount !== colCount) {
            throw new Error("Invalid number of characters in map row");
          }
          rowCount++;
          colCount = 0;
          prevChar = ch;
          break;
        case " ":
        case "\t":
          if (
            mapReaderState === MR_READ_PLAYERID ||
            mapReaderState === MR_READ_INITIAL_ORIENTATION
          ) {
            this.addTile(
              colCount,
              rowCount,
              currentObjectName,
              Number(currentPlayerIdChars.join()),
              currentDirection,
              false
            );
            mapReaderState = MR_READ_NORMAL;
            currentDirection = "NONE";
            currentPlayerIdChars = [];
            colCount++;
          }
          break;
        case ".": // dots just signify an empty space
          if (
            mapReaderState === MR_READ_PLAYERID ||
            mapReaderState === MR_READ_INITIAL_ORIENTATION
          ) {
            this.addTile(
              colCount,
              rowCount,
              currentObjectName,
              Number(currentPlayerIdChars.join()),
              currentDirection,
              false
            );
            mapReaderState = MR_READ_NORMAL;
            currentDirection = "NONE";
            currentPlayerIdChars = [];
            colCount++;
          }
          // Add the bg tile also
          this.addTile(
            colCount,
            rowCount,
            "background",
            0,
            "NONE",
            false
          );

          colCount++;
          prevChar = ch;
          break;
        case "/":
          if (
            mapReaderState === MR_READ_PLAYERID ||
            mapReaderState === MR_READ_INITIAL_ORIENTATION
          ) {
            this.addTile(
              colCount,
              rowCount,
              currentObjectName,
              Number(currentPlayerIdChars.join()),
              currentDirection,
              false
            );
            currentPlayerIdChars = [];
            mapReaderState = MR_READ_NORMAL;
            currentDirection = "NONE";
          }
          prevChar = ch;
          break;
        case "[":
          if (mapReaderState === MR_READ_PLAYERID) {
            mapReaderState = MR_READ_INITIAL_ORIENTATION;
          }
          prevChar = ch;
          break;

        case "]":
          if (mapReaderState !== MR_READ_INITIAL_ORIENTATION) {
            throw new Error(
              `Invalid closing bracket ']' for initial orientation in map row=${rowCount}`
            );
          }
          prevChar = ch;
          break;
        default: {
          switch (mapReaderState) {
            case MR_READ_NORMAL:
              currentObjectName = this.characterToObject[ch];
              mapReaderState = MR_READ_PLAYERID;
              break;
            case MR_READ_PLAYERID:
              if (!isNaN(ch)) {
                currentPlayerIdChars.push(ch);
              } else {
                this.addTile(
                  colCount,
                  rowCount,
                  currentObjectName,
                  Number(currentPlayerIdChars.join()),
                  currentDirection,
                  false
                );
                currentObjectName = this.characterToObject[ch];
                currentDirection = "NONE";
                currentPlayerIdChars = [];
                colCount++;
              }
              break;
            case MR_READ_INITIAL_ORIENTATION:
              switch (ch) {
                case "U":
                  currentDirection = "UP";
                  break;
                case "D":
                  currentDirection = "DOWN";
                  break;
                case "L":
                  currentDirection = "LEFT";
                  break;
                case "R":
                  currentDirection = "RIGHT";
                  break;
                default:
                  throw new Error(
                    `Unknown direction character ${ch} at in map row=${rowCount}`
                  );
              }
              break;
            default:
              throw new Error(
                "Unknown state reached when parsing level string"
              );
          }
          prevChar = ch;
          break;
        }
      }

      charIdx++;
    }

    if (
      mapReaderState === MR_READ_PLAYERID ||
      mapReaderState === MR_READ_INITIAL_ORIENTATION
    ) {
      this.addTile(
        colCount,
        rowCount,
        currentObjectName,
        Number(currentPlayerIdChars.join()),
        currentDirection,
        false
      );
      currentPlayerIdChars = [];
      currentDirection = "NONE";
      mapReaderState = MR_READ_NORMAL;
    }

    if (prevChar !== "\n") {
      rowCount += 1;
    }

    // only run the onLevelString at the end of loading the level
    const newLevelString = this.toLevelString(this.getState());
    if (this.onLevelString) {
      this.onLevelString(newLevelString);
    }

  }

  updateStateSize(state) {
    state.minx = 0;
    state.miny = 0;
    state.maxx = 3;
    state.maxy = 3;

    for (const objectLocationKey in state.objects) {
      for (const objectId in state.objects[objectLocationKey]) {
        const objectInfo = state.objects[objectLocationKey][objectId];

        if (objectInfo.location.x < state.minx) {
          state.minx = objectInfo.location.x;
        } else if (objectInfo.location.x > state.maxx) {
          state.maxx = objectInfo.location.x;
        }

        if (objectInfo.location.y < state.miny) {
          state.miny = objectInfo.location.y;
        } else if (objectInfo.location.y > state.maxy) {
          state.maxy = objectInfo.location.y;
        }
      }
    }

    state.gridWidth = Math.max(
      this.minimumStateWidth,
      state.maxx - state.minx + 1
    );
    state.gridHeight = Math.max(
      this.minimumStateHeight,
      state.maxy - state.miny + 1
    );
    return state;
  }

  pushState = (state, notify = true) => {
    // Copy the state and add it to the history
    const stateCopy = {
      ...this.updateStateSize(state),
    };

    const levelString = this.toLevelString(stateCopy);

    const hashedStateCopy = {
      ...stateCopy,
      hash: hashString(levelString),
    };

    this.editorHistory.push(hashedStateCopy);

    const historyLength = this.editorHistory.length;

    if (historyLength >= 20) {
      this.editorHistory.shift();
    }

    if (this.onLevelString && notify) {
      this.onLevelString(levelString);
    }
  };

  addTile(x, y, objectName, playerId, orientation, notify = true) {
    let state = this.getState();

    const charAndZ = this.objectToCharacterAndZ[objectName];

    const objectInfo = {
      id: getRandomHash(),
      renderTileId: 0,
      name: objectName,
      char: charAndZ.char,
      playerId,
      orientation,
      location: { x, y, z: charAndZ.z },
    };

    const locationKey = this.getObjectLocationKey(x, y);

    if (!(locationKey in state.objects)) {
      state.objects[locationKey] = [];
    } else {
      // Remove existing object with same z location
      for (const k in state.objects[locationKey]) {
        const object = state.objects[locationKey][k];

        if (object.location.z === objectInfo.location.z) {
          state.tileTypeCount[objectInfo.name]--;
          state.objects[locationKey].splice(k, 1);
        }
      }
    }

    // Add new object
    state.objects[locationKey].push(objectInfo);

    // Sort by Z location
    state.objects[locationKey].sort((a, b) => b.location.z - a.location.z);

    if (!(objectInfo.name in state.tileTypeCount)) {
      state.tileTypeCount[objectInfo.name] = 0;
    }
    state.tileTypeCount[objectInfo.name]++;

    this.pushState(state, notify);
  }

  removeTile(x, y) {
    const state = this.getState();

    const locationKey = this.getObjectLocationKey(x, y);

    if (locationKey in state.objects) {
      const objectInfo = state.objects[locationKey][0];
      state.tileTypeCount[objectInfo.name]--;
      state.objects[locationKey].splice(0, 1);

      if (state.objects[locationKey].length === 0) {
        delete state.objects[locationKey];
        console.log("removing key");
      }

      this.pushState(state);
    }
  }

  getState() {
    return { ...this.editorHistory[this.editorHistory.length - 1] };
  }

  toLevelString(state) {
    const levelObjectChars = [];

    let maxObjectChars = this.minimumObjectChars;

    for (let y = state.miny; y <= state.maxy; y++) {
      for (let x = state.minx; x <= state.maxx; x++) {
        const locationKey = this.getObjectLocationKey(x, y);
        if (locationKey in state.objects) {
          const locationChars = [];
          for (const k in state.objects[locationKey]) {
            const objectInfo = state.objects[locationKey][k];
            if(objectInfo.char !== ".") {
              locationChars.push(objectInfo.char);
            }
          }
          let objectChars;
          if (locationChars.length === 0) {
            objectChars = "."
          } else {
            objectChars = locationChars.join("/");
          }

          if (objectChars.length > maxObjectChars) {
            maxObjectChars = objectChars.length;
          }

          levelObjectChars.push(objectChars);
        } else {
          levelObjectChars.push(".");
        }
      }
      levelObjectChars.push("\n");
    }

    return levelObjectChars
      .map((chars) => {
        if (chars !== "\n") {
          return chars.padEnd(maxObjectChars + 1);
        }
        return chars;
      })
      .join("");
  }
}

export default EditorStateHandler;
