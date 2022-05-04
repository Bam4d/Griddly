const MR_READ_NORMAL = 0;
const MR_READ_PLAYERID = 1;
const MR_READ_INITIAL_ORIENTATION = 2;

class EditorStateHandler {
  constructor(gdy) {
    this.defaultTileSize = 24;
    this.objectTemplates = {};

    this.characterToObject = {};
    this.objectToCharacter = {};

    this.gdy = gdy;

    this.editorHistory = [];

    this.loadObjects(gdy);

    this.initialState = {
      objects: {},
      gridWidth: 0,
      gridHeight: 0,
      minx: Number.MAX_VALUE,
      miny: Number.MAX_VALUE,
      maxx: Number.MIN_VALUE,
      maxy: Number.MIN_VALUE,
      tileTypeCount: {},
    };

    // Just used to create a random id
    this.objectId = 0;

    // Another simple way to create a unique id for consecutive states so we dont always re-draw unecessarily
    this.hash = 0;

    this.pushState(this.initialState);
  }

  getObjectLocationKey = (x, y) => {
    return `${x},${y}`;
  };

  loadObjects(gdy) {
    this.characterToObject["."] = "background";
    this.objectToCharacter["background"] = ".";

    gdy.Objects.forEach((object) => {
      this.characterToObject[object.MapCharacter] = object.Name;
      this.objectToCharacter[object.Name] = object.MapCharacter;
    });
  }

  loadLevelString(levelString) {
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
              currentDirection
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
              currentDirection
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
              currentDirection
            );
            mapReaderState = MR_READ_NORMAL;
            currentDirection = "NONE";
            currentPlayerIdChars = [];
            colCount++;
          }
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
              currentDirection
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
                  currentDirection
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
        currentDirection
      );
      currentPlayerIdChars = [];
      currentDirection = "NONE";
      mapReaderState = MR_READ_NORMAL;
    }

    if (prevChar !== "\n") {
      rowCount += 1;
    }
  }

  updateStateSize(state) {
    state.minx = Number.MAX_VALUE;
    state.miny = Number.MAX_VALUE;
    state.maxx = Number.MIN_VALUE;
    state.maxy = Number.MIN_VALUE;

    for (const objectId in state.objects) {
      const objectInfo = state.objects[objectId];

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

    state.gridWidth = state.maxx - state.minx + 1;
    state.gridHeight = state.maxy - state.miny + 1;
    return state;
  }

  pushState = (state) => {
    // Copy the state and add it to the history
    const stateCopy = {
      ...this.updateStateSize(state),
      hash: this.hash++,
    };

    this.editorHistory.push(stateCopy);

    const historyLength = this.editorHistory.length;

    if (historyLength >= 20) {
      this.editorHistory.shift();
    }

    console.log(this.toLevelString(stateCopy));
  };

  addTile(x, y, objectName, playerId, orientation) {
    let state = this.getState();

    const objectInfo = {
      id: this.objectId++,
      renderTileId: 0,
      name: objectName,
      char: this.objectToCharacter[objectName],
      playerId,
      orientation,
      location: { x, y },
    };

    state.objects[this.getObjectLocationKey(x, y)] = objectInfo;

    if (!(objectInfo.name in state.tileTypeCount)) {
      state.tileTypeCount[objectInfo.name] = 0;
    }
    state.tileTypeCount[objectInfo.name]++;

    this.pushState(state);
  }

  removeTile(x, y) {
    const state = this.getState();

    const locationKey = this.getObjectLocationKey(x, y);

    if (locationKey in state.objects) {
      const objectInfo = state.objects[locationKey];
      state.tileTypeCount[objectInfo.name]--;
      delete state.objects[locationKey];

      this.pushState(state);
    }
  }

  getState() {
    return { ...this.editorHistory[this.editorHistory.length - 1] };
  }

  toLevelString(state) {

    const levelObjectChars = [];

    for(let x = state.minx; x<state.maxx; x++) {
      for(let y = state.miny; y<state.maxy; y++) {
        const locationKey = this.getObjectLocationKey(x, y);
        if (locationKey in state.objects) {
          const objectInfo = state.objects[locationKey];

          levelObjectChars.push(objectInfo.char);
        } else {
          levelObjectChars.push(".");
        }
      }
    }

    return levelObjectChars.join(" ");
  }
}

export default EditorStateHandler;
