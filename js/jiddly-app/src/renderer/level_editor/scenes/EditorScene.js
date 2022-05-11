import Phaser from "phaser";

import Block2DRenderer from "../../Block2DRenderer";
import Sprite2DRenderer from "../../Sprite2DRenderer";
import {
  COLOR_LOADING_TEXT,
  COLOR_PANEL_DARK,
  COLOR_PANEL_LIGHT,
  COLOR_SELECT_TILE_TEXT,
  COLOR_SELECT,
  COLOR_PLACE,
  COLOR_FOREGROUND,
  COLOR_SELECT_MOVE_TOOL_TEXT,
  COLOR_SELECT_SELECT_TOOL_TEXT,
  COLOR_SELECT_PLACE_TOOL_TEXT,
  COLOR_SELECT_TILE_HIGHLIGHTED,
} from "../../ThemeConsts";

const selectTileBoxHeight = 30;
const selectTileBoxPadding = 3;

const selectToolBoxPadding = 50;
const selectToolBoxWidth = 35;
const selectToolBoxHeight = 35;

const selectTileSpriteOffset = 3;

const toolBoxYOffset = 50;
const tileBoxYOffset = 100;

class EditorScene extends Phaser.Scene {
  constructor() {
    super("EditorScene");

    this.editorCenterX = 0;
    this.editorCenterY = 0;

    this.origDragPoint = null;

    this.editorGridLocation = {
      x: 0,
      y: 0,
    };

    this.selectTileBgMap = {};
  }

  createTileMenu() {
    // Make a rectangle on the left and add more rectangles to it
    this.selectTilePanel = this.add.rectangle(
      0,
      0,
      this.cameras.main.width / 5,
      this.cameras.main.height,
      COLOR_PANEL_DARK
    );

    this.selectTilePanel.setOrigin(0, 0);
    this.selectTilePanel.setDepth(200);

    const selectToolButtonOffset = this.selectTilePanel.width / 2;

    const moveToolButtonX = selectToolButtonOffset - selectToolBoxPadding;
    const selectToolButtonX = selectToolButtonOffset;
    const placeToolButtonX = selectToolButtonOffset + selectToolBoxPadding;

    // Move Tool Box
    this.selectMoveToolBg = this.add.rectangle(
      moveToolButtonX,
      toolBoxYOffset,
      selectToolBoxWidth,
      selectToolBoxHeight,
      COLOR_PANEL_LIGHT
    );
    this.selectMoveToolBg.setDepth(201);
    this.selectMoveToolBg.setInteractive();
    this.selectMoveToolBg.on("pointerdown", () => this.selectTool("move"));
    this.add
      .text(moveToolButtonX, toolBoxYOffset, "\uf047", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_SELECT_MOVE_TOOL_TEXT,
        fontSize: "24px",
      })
      .setDepth(202)
      .setOrigin(0.5, 0.5);

    // Select Tool Box
    this.selectSelectToolBg = this.add.rectangle(
      selectToolButtonX,
      toolBoxYOffset,
      selectToolBoxWidth,
      selectToolBoxHeight,
      COLOR_PANEL_LIGHT
    );
    this.selectSelectToolBg.setDepth(201);
    this.selectSelectToolBg.setInteractive();
    this.selectSelectToolBg.on("pointerdown", () => this.selectTool("select"));
    this.add
      .text(selectToolButtonX, toolBoxYOffset, "\uf245", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_SELECT_SELECT_TOOL_TEXT,
        fontSize: "24px",
      })
      .setDepth(202)
      .setOrigin(0.5, 0.5);

    // Place Tool Box
    this.selectPlaceToolBg = this.add.rectangle(
      placeToolButtonX,
      toolBoxYOffset,
      selectToolBoxWidth,
      selectToolBoxHeight,
      COLOR_PANEL_LIGHT
    );
    this.selectPlaceToolBg.setDepth(201);
    this.selectPlaceToolBg.setInteractive();
    this.selectPlaceToolBg.on("pointerdown", () => this.selectTool("place"));
    this.add
      .text(placeToolButtonX, toolBoxYOffset, "\uf1fc", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_SELECT_PLACE_TOOL_TEXT,
        fontSize: "24px",
      })
      .setDepth(202)
      .setOrigin(0.5, 0.5);

    const objectNames = new Set();
    let o = 0;
    for (const objectTemplateName in this.renderer.objectTemplates) {
      const objectTemplate = this.renderer.objectTemplates[objectTemplateName];

      if (!objectNames.has(objectTemplate.name)) {
        objectNames.add(objectTemplate.name);

        const selectTileBg = this.add.rectangle(
          this.selectTilePanel.x + selectTileBoxPadding,
          tileBoxYOffset +
            this.selectTilePanel.y +
            o * (selectTileBoxPadding * 2 + selectTileBoxHeight),
          this.selectTilePanel.width - 3 * selectTileBoxPadding,
          selectTileBoxHeight,
          COLOR_PANEL_LIGHT
        );
        selectTileBg.setOrigin(0, 0);
        selectTileBg.setDepth(201);
        selectTileBg.setInteractive();
        selectTileBg.on("pointerdown", () => this.selectTile(objectTemplate));

        this.selectTileBgMap[objectTemplate.id] = selectTileBg;

        const selectTileTopRight = selectTileBg.getTopRight();
        const selectTileTopLeft = selectTileBg.getTopLeft();

        // Create a sprite and add it to the list .. if there are multiple tiles we can cycle through them on mouseover
        const selectTileText = this.add.text(
          selectTileTopLeft.x + selectTileSpriteOffset,
          selectTileTopLeft.y + selectTileSpriteOffset,
          objectTemplate.name,
          {
            fontFamily: "Font Awesome Regular",
            color: COLOR_SELECT_TILE_TEXT,
            font: "16px",
          }
        );
        selectTileText.setOrigin(0, 0);
        selectTileText.setDepth(202);
        const selectTileSprite = this.add.sprite(
          selectTileTopRight.x -
            this.renderConfig.TileSize / 2.0 -
            selectTileSpriteOffset,
          selectTileTopRight.y +
            this.renderConfig.TileSize / 2.0 +
            selectTileSpriteOffset,
          this.renderer.getTilingImage(objectTemplate, -1, -1)
        );
        selectTileSprite.setOrigin(0.5, 0.5);
        selectTileSprite.setDisplaySize(
          this.renderConfig.TileSize * objectTemplate.scale,
          this.renderConfig.TileSize * objectTemplate.scale
        );
        selectTileSprite.setTint(
          Phaser.Display.Color.GetColor(
            objectTemplate.color.r * 255,
            objectTemplate.color.g * 255,
            objectTemplate.color.b * 255
          )
        );
        selectTileSprite.setDepth(202);

        o++;
      }
    }
  }

  displayError = (error) => {
    console.log("Display Error: ", error);
  };

  loadRenderers() {
    const observers = this.gdy.Environment.Observers;

    this.rendererConfigs = [];

    for (const rendererName in observers) {
      const rendererConfig = observers[rendererName];

      if (!("TileSize" in rendererConfig)) {
        rendererConfig["TileSize"] = this.defaultTileSize;
      }

      if (!("Type" in rendererConfig)) {
        if (rendererName === "SPRITE_2D" || rendererName === "Sprite2D") {
          rendererConfig["Type"] = "SPRITE_2D";
        } else if (rendererName === "BLOCK_2D" || rendererName === "Block2D") {
          rendererConfig["Type"] = "BLOCK_2D";
        } else {
          this.displayError(
            "Only Block2D and Sprite2D renderers can be used to view Jiddly environments"
          );
        }
      }

      this.rendererConfigs[rendererName] = rendererConfig;
    }
  }

  init = (data) => {
    this.gdy = data.gdy;

    this.editorStateHandler = data.editorStateHandler;

    this.avatarObject = this.gdy.Environment.Player.AvatarObject;

    this.rendererName = data.rendererName;

    this.loadRenderers();

    this.renderConfig = this.rendererConfigs[this.rendererName];
    this.avatarObject = this.gdy.Environment.Player.AvatarObject;

    if (this.renderConfig.Type === "BLOCK_2D") {
      this.renderer = new Block2DRenderer(
        this,
        this.rendererName,
        this.renderConfig,
        this.avatarObject
      );
    } else if (this.renderConfig.Type === "SPRITE_2D") {
      this.renderer = new Sprite2DRenderer(
        this,
        this.rendererName,
        this.renderConfig,
        this.avatarObject
      );
    }

    this.renderData = {
      objects: {},
    };

    this.currentTool = "select";
  };

  updateState = (state) => {
    const objectList = [];

    for (const objectsAtLocation in state.objects) {
      for (const object in state.objects[objectsAtLocation]) {
        objectList.push(state.objects[objectsAtLocation][object]);
      }
    }

    const newObjectIds = objectList.map((object) => {
      return object.id;
    });

    this.renderer.beginUpdate(objectList, state);

    objectList.forEach((object) => {
      const objectTemplateName = object.name + object.renderTileId;
      if (object.id in this.renderData.objects) {
        const currentObjectData = this.renderData.objects[object.id];
        this.renderer.updateObject(
          currentObjectData.sprite,
          object.name,
          objectTemplateName,
          object.location.x,
          object.location.y,
          object.orientation
        );

        this.renderData.objects[object.id] = {
          ...currentObjectData,
          object,
        };
      } else {
        const sprite = this.renderer.addObject(
          object.name,
          objectTemplateName,
          object.location.x,
          object.location.y,
          object.orientation
        );

        this.renderData.objects[object.id] = {
          object,
          sprite,
        };
      }
    });

    for (const k in this.renderData.objects) {
      const id = this.renderData.objects[k].object.id;
      if (!newObjectIds.includes(id)) {
        this.renderData.objects[k].sprite.destroy();
        delete this.renderData.objects[k];
      }
    }

    this.editorGridBounds = {
      origX: this.renderer.getCenteredX(-3.5),
      origY: this.renderer.getCenteredY(-3.5),
      x: this.renderer.getCenteredX(state.minx - 3.5),
      y: this.renderer.getCenteredY(state.miny - 3.5),
      width: (state.gridWidth + 6) * this.renderConfig.TileSize,
      height: (state.gridHeight + 6) * this.renderConfig.TileSize,
    };

    if (this.grid) {
      this.grid.setPosition(this.editorGridBounds.x, this.editorGridBounds.y);

      this.grid.width = this.editorGridBounds.width;
      this.grid.height = this.editorGridBounds.height;
    }

    this.editorContainer.sort("depth");
  };

  selectTile = (objectTemplate) => {
    if (this.currentTool !== "place") {
      this.selectTool("place");
    }
    console.log("Select Tile", objectTemplate);
    this.selectedTile = objectTemplate;

    if (!this.placeTileOverlay) {
      this.placeTileOverlay = this.add.sprite(
        this.editorGridLocation.x,
        this.editorGridLocation.y,
        this.selectedTile.id
      );
      this.placeTileOverlay.on("pointerdown", this.handlePlaceAction);
      this.editorContainer.add(this.placeTileOverlay);
    }

    this.placeTileOverlay.setTexture(
      this.renderer.getTilingImage(objectTemplate, -1, -1)
    );

    this.placeTileOverlay.setDepth(50);
    this.placeTileOverlay.setAlpha(0.5);
    this.placeTileOverlay.setOrigin(0.5, 0.5);
    this.placeTileOverlay.setInteractive();
    this.placeTileOverlay.setDisplaySize(
      objectTemplate.scale * this.renderConfig.TileSize,
      objectTemplate.scale * this.renderConfig.TileSize
    );
    this.placeTileOverlay.setTint(
      Phaser.Display.Color.GetColor(
        objectTemplate.color.r * 255,
        objectTemplate.color.g * 255,
        objectTemplate.color.b * 255
      )
    );

    for (const selectTileBgId in this.selectTileBgMap) {
      const selectTileBg = this.selectTileBgMap[selectTileBgId];

      if (selectTileBgId === objectTemplate.id) {
        selectTileBg.setStrokeStyle(3, COLOR_SELECT_TILE_HIGHLIGHTED);
      } else {
        selectTileBg.setStrokeStyle(0);
      }
    }
  };

  selectTool = (toolName) => {
    console.log("Select Tool", toolName);
    this.currentTool = toolName;
    switch (this.currentTool) {
      case "move":
        this.selectMoveToolBg.setStrokeStyle(3, COLOR_SELECT_TILE_HIGHLIGHTED);
        this.selectSelectToolBg.setStrokeStyle(0);
        this.selectPlaceToolBg.setStrokeStyle(0);
        this.selectRectangle.setActive(false).setVisible(false);
        this.placeRectangle.setActive(false).setVisible(false);
        if (this.placeTileOverlay) {
          this.placeTileOverlay.destroy();
          this.placeTileOverlay = null;
        }
        this.selectedTile = null;
        for (const selectTileBgId in this.selectTileBgMap) {
          this.selectTileBgMap[selectTileBgId].setStrokeStyle(0);
        }
        break;
      case "select":
        this.selectMoveToolBg.setStrokeStyle(0);
        this.selectSelectToolBg.setStrokeStyle(
          3,
          COLOR_SELECT_TILE_HIGHLIGHTED
        );
        this.selectPlaceToolBg.setStrokeStyle(0);
        this.selectRectangle.setActive(true).setVisible(true);
        this.placeRectangle.setActive(false).setVisible(false);
        if (this.placeTileOverlay) {
          this.placeTileOverlay.destroy();
          this.placeTileOverlay = null;
        }
        this.selectedTile = null;
        for (const selectTileBgId in this.selectTileBgMap) {
          this.selectTileBgMap[selectTileBgId].setStrokeStyle(0);
        }
        break;
      case "place":
        this.selectMoveToolBg.setStrokeStyle(0);
        this.selectSelectToolBg.setStrokeStyle(0);
        this.selectPlaceToolBg.setStrokeStyle(3, COLOR_SELECT_TILE_HIGHLIGHTED);
        this.selectRectangle.setActive(false).setVisible(false);
        this.placeRectangle.setVisible(true).setActive(true);
        break;
      default:
        break;
    }
  };

  handlePlaceAction = (pointer) => {
    if (pointer.rightButtonDown()) {
      this.removeObject();
    } else {
      this.placeObject();
    }
  };

  removeObject = () => {
    this.editorStateHandler.removeTile(
      this.editorGridLocation.relX - 3,
      this.editorGridLocation.relY - 3
    );
  };

  placeObject = () => {
    if (this.selectedTile) {
      this.editorStateHandler.addTile(
        this.editorGridLocation.relX - 3,
        this.editorGridLocation.relY - 3,
        this.selectedTile.name,
        0,
        "NONE"
      );
    }
  };

  selectObject = () => {};

  mouseMoved = (x, y) => {
    const editorX = x - this.editorCenterX;
    const editorY = y - this.editorCenterY;

    if (
      editorX >= this.editorGridBounds.x &&
      editorY >= this.editorGridBounds.y &&
      editorX < this.editorGridBounds.x + this.editorGridBounds.width &&
      editorY < this.editorGridBounds.y + this.editorGridBounds.height
    ) {
      // Calculate grid location
      this.editorGridLocation = {
        relX: Math.floor(
          (editorX - this.editorGridBounds.origX) / this.renderConfig.TileSize
        ),
        relY: Math.floor(
          (editorY - this.editorGridBounds.origY) / this.renderConfig.TileSize
        ),
        x: Math.floor(
          (editorX - this.editorGridBounds.x) / this.renderConfig.TileSize
        ),
        y: Math.floor(
          (editorY - this.editorGridBounds.y) / this.renderConfig.TileSize
        ),
      };

      switch (this.currentTool) {
        case "move":
          if (this.input.activePointer.isDown) {
            if (this.origDragPoint) {
              // move the camera by the amount the mouse has moved since last update
              this.editorCenterX +=
                this.input.activePointer.position.x - this.origDragPoint.x;
              this.editorCenterY +=
                this.input.activePointer.position.y - this.origDragPoint.y;

              this.editorContainer.setPosition(
                this.editorCenterX,
                this.editorCenterY
              );
            } // set new drag origin to current position
            this.origDragPoint = this.input.activePointer.position.clone();
          } else {
            this.origDragPoint = null;
          }
          break;
        case "select":
          this.selectRectangle.setPosition(
            this.editorGridBounds.x +
              this.editorGridLocation.x * this.renderConfig.TileSize,
            this.editorGridBounds.y +
              this.editorGridLocation.y * this.renderConfig.TileSize
          );
          break;
        case "place":
          if (this.input.activePointer.isDown) {
            this.handlePlaceAction(this.input.activePointer);
          }

          this.placeRectangle.setPosition(
            this.editorGridBounds.x +
              this.editorGridLocation.x * this.renderConfig.TileSize,
            this.editorGridBounds.y +
              this.editorGridLocation.y * this.renderConfig.TileSize
          );
          if (this.placeTileOverlay) {
            this.placeTileOverlay.setPosition(
              this.editorGridBounds.x +
                this.editorGridLocation.x * this.renderConfig.TileSize + this.renderConfig.TileSize/2.0,
              this.editorGridBounds.y +
                this.editorGridLocation.y * this.renderConfig.TileSize + this.renderConfig.TileSize/2.0
            );
          }
          break;
        default:
          break;
      }
    }
  };

  configureEditGrid = () => {
    this.input.on("pointermove", (pointer) => {
      this.mouseMoved(pointer.x, pointer.y);
    });

    this.selectRectangle = this.add.rectangle(
      0,
      0,
      this.renderConfig.TileSize,
      this.renderConfig.TileSize
    );
    this.selectRectangle.setStrokeStyle(2, COLOR_SELECT, 0.5);
    this.selectRectangle.setDepth(100);
    this.selectRectangle.setOrigin(0, 0);
    this.selectRectangle.setInteractive();
    this.selectRectangle.on("pointerdown", () => this.selectObject());

    this.placeRectangle = this.add.rectangle(
      0,
      0,
      this.renderConfig.TileSize,
      this.renderConfig.TileSize
    );
    this.placeRectangle.setStrokeStyle(2, COLOR_PLACE, 0.5);
    this.placeRectangle.setDepth(100);
    this.placeRectangle.setOrigin(0, 0);
    this.placeRectangle.setVisible(false);
    //this.placeRectangle.setInteractive();
    //this.placeRectangle.on("pointerdown", () => this.placeObject());

    this.grid = this.add.grid(
      this.editorGridBounds.x,
      this.editorGridBounds.y,
      this.editorGridBounds.width,
      this.editorGridBounds.height,
      this.renderConfig.TileSize,
      this.renderConfig.TileSize
    );

    this.grid.setOutlineStyle(COLOR_FOREGROUND, 0.2);
    this.grid.setDepth(50);
    this.grid.setOrigin(0, 0);

    this.editorContainer.add(this.selectRectangle);
    this.editorContainer.add(this.placeRectangle);
    this.editorContainer.add(this.grid);
  };

  preload() {
    console.log("Editor Scene - Preload");

    this.input.mouse.disableContextMenu();

    this.loadingText = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading Level Editor",
      {
        fontFamily: "Droid Sans Mono",
        fill: COLOR_LOADING_TEXT,
        align: "center",
      }
    );

    this.loadingText.setX(this.cameras.main.width / 2);
    this.loadingText.setY(this.cameras.main.height / 2);
    this.loadingText.setOrigin(0.5, 0.5);

    if (this.renderer) {
      this.renderer.loadTemplates(this.gdy.Objects);
    }
  }

  create() {
    console.log("Editor Scene - Create");

    this.loadingText.destroy();
    this.loaded = true;

    this.editorContainer = this.add.container(
      this.editorCenterX,
      this.editorCenterY
    );

    if (this.renderer) {
      const state = this.editorStateHandler.getState();
      this.renderer.init(
        state.gridWidth,
        state.gridHeight,
        this.editorContainer
      );
      this.updateState(state);
      this.configureEditGrid();
      this.createTileMenu();
    }
  }

  update() {
    if (!this.loaded) {
      this.loadingText.setX(this.cameras.main.width / 2);
      this.loadingText.setY(this.cameras.main.height / 2);
      this.loadingText.setOrigin(0.5, 0.5);
    } else {
      if (this.renderer) {
        const state = this.editorStateHandler.getState();
        if (state && this.stateHash !== state.hash) {
          this.stateHash = state.hash;
          this.updateState(state);
        }
      }
    }
  }
}

export default EditorScene;
