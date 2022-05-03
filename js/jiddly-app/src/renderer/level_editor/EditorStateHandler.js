const MR_READ_NORMAL = 0;
const MR_READ_PLAYERID = 1;
const MR_READ_INITIAL_ORIENTATION = 2;

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
      gridWidth: 0,
      gridHeight: 0,
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

    gdy.Objects.forEach((object) => {
      this.characterToObject[object.MapCharacter] = object.Name;
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
            throw new Error(`Invalid closing bracket ']' for initial orientation in map row=${rowCount}`);
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
                  throw new Error(`Unknown direction character ${ch} at in map row=${rowCount}`);
              }
              break;
            default:
              throw new Error("Unknown state reached when parsing level string");
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

  pushState = (state) => {
    // Copy the state and add it to the history
    const stateCopy = {
      ...state,
      hash: this.hash++,
    };
    this.editorHistory.push(stateCopy);

    const historyLength = this.editorHistory.length;

    if (historyLength >= 20) {
      this.editorHistory.shift();
    }
  };

  addTile(x, y, objectName, playerId, orientation) {
    const state = this.getState();

    const objectInfo = {
      id: this.objectId++,
      renderTileId: 0,
      name: objectName,
      playerId,
      orientation,
      location: { x, y },
    };

    if (state.gridWidth < x) {
      state.gridWidth = x;
    }

    if (state.gridHeight < y) {
      state.gridHeight = y;
    }

    state.objects[this.getObjectLocationKey(x, y)] = objectInfo;

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
    return { ...this.editorHistory[this.editorHistory.length - 1] };
  }

  toLevelString() {}
}

export default EditorStateHandler;
