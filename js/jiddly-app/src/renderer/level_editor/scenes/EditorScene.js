import Phaser from "phaser";

import EditorStateHandler from "../EditorStateHandler";
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
    const selectMoveToolBg = this.add.rectangle(
      moveToolButtonX,
      toolBoxYOffset,
      selectToolBoxWidth,
      selectToolBoxHeight,
      COLOR_PANEL_LIGHT
    );
    selectMoveToolBg.setDepth(201);
    selectMoveToolBg.setInteractive();
    selectMoveToolBg.on("pointerdown", () => this.selectTool("move"));

    // Select Tool Box
    const selectSelectToolBg = this.add.rectangle(
      selectToolButtonX,
      toolBoxYOffset,
      selectToolBoxWidth,
      selectToolBoxHeight,
      COLOR_PANEL_LIGHT
    );
    selectSelectToolBg.setDepth(201);
    selectSelectToolBg.setInteractive();
    selectSelectToolBg.on("pointerdown", () => this.selectTool("select"));

    // Place Tool Box
    const selectPlaceToolBg = this.add.rectangle(
      placeToolButtonX,
      toolBoxYOffset,
      selectToolBoxWidth,
      selectToolBoxHeight,
      COLOR_PANEL_LIGHT
    );
    selectPlaceToolBg.setDepth(201);
    selectPlaceToolBg.setInteractive();
    selectPlaceToolBg.on("pointerdown", () => this.selectTool("place"));

    let o = 0;
    for (const objectTemplateName in this.renderer.objectTemplates) {
      const objectTemplate = this.renderer.objectTemplates[objectTemplateName];

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

      const selectTileTopRight = selectTileBg.getTopRight();
      const selectTileTopLeft = selectTileBg.getTopLeft();

      // Create a sprite and add it to the list .. if there are multiple tiles we can cycle through them on mouseover
      const selectTileText = this.add.text(
        selectTileTopLeft.x + selectTileSpriteOffset,
        selectTileTopLeft.y + selectTileSpriteOffset,
        objectTemplate.name,
        {
          fontFamily: "Droid Sans Mono",
          color: COLOR_SELECT_TILE_TEXT,
          font: "16px",
        }
      );
      selectTileText.setOrigin(0, 0);
      selectTileText.setDepth(202);
      const selectTileSprite = this.add.sprite(
        selectTileTopRight.x -
          this.renderConfig.TileSize -
          selectTileSpriteOffset,
        selectTileTopRight.y + selectTileSpriteOffset,
        objectTemplate.id
      );
      selectTileSprite.setOrigin(0, 0);
      selectTileSprite.setDepth(202);

      o++;
    }
  }

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

    this.editorStateHandler = new EditorStateHandler(this.gdy);
    const levels = this.gdy.Environment.Levels;
    this.editorStateHandler.loadLevelString(levels[data.levelId]);

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

    for (const object in state.objects) {
      objectList.push(state.objects[object]);
    }

    const newObjectIds = objectList.map((object) => {
      return object.id;
    });

    this.renderer.beginUpdate(objectList);

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
      x: this.renderer.getCenteredX(-3.5),
      y: this.renderer.getCenteredY(-3.5),
      width: (this.renderer.gridWidth + 7) * this.renderConfig.TileSize,
      height: (this.renderer.gridHeight + 7) * this.renderConfig.TileSize,
    };
  };

  selectTile = (objectTemplate) => {
    console.log("Select Tile", objectTemplate);
    this.selectedTile = objectTemplate;

    if (this.placeTileOverlay) {
      this.placeTileOverlay.setTexture(this.selectedTile.id);
    } else {
      this.placeTileOverlay = this.add.sprite(
        this.editorGridLocation.x,
        this.editorGridLocation.y,
        this.selectedTile.id
      );

      this.placeTileOverlay.setDepth(50);
      this.placeTileOverlay.setAlpha(0.5);
      this.placeTileOverlay.setOrigin(0, 0);
      this.placeTileOverlay.setInteractive();
      this.placeTileOverlay.on("pointerdown", () => this.placeObject());
      this.editorContainer.add(this.placeTileOverlay);
    }
  };

  selectTool = (toolName) => {
    console.log("Select Tool", toolName);
    this.currentTool = toolName;
    switch (this.currentTool) {
      case "move":
        this.selectRectangle.setActive(false).setVisible(false);

        if (this.placeTileOverlay) {
          this.placeTileOverlay.setActive(false).setVisible(false);
        }
        break;
      case "select":
        this.selectRectangle.setActive(true).setVisible(true);
        this.placeRectangle.setActive(false).setVisible(false);
        if (this.placeTileOverlay) {
          this.placeTileOverlay.setActive(false).setVisible(false);
        }
        break;
      case "place":
        this.selectRectangle.setActive(false).setVisible(false);
        this.placeRectangle.setVisible(true).setActive(true);
        if (this.placeTileOverlay) {
          this.placeTileOverlay.setActive(true).setVisible(true);
        }
        break;
      default:
        break;
    }
  };

  placeObject = () => {
    if (this.selectedTile) {
      console.log("Object placed", this.selectedTile);
      console.log(this.editorGridLocation);
      this.editorStateHandler.addTile(
        this.editorGridLocation.x - 3,
        this.editorGridLocation.y - 3,
        this.selectedTile.name,
        0,
        "NONE"
      );
    }
  };

  selectObject = () => {};

  mouseMoved = (x, y) => {
    const editorX = x - this.editorGridBounds.x - this.editorCenterX;
    const editorY = y - this.editorGridBounds.y - this.editorCenterY;

    if (
      editorX >= 0 &&
      editorY >= 0 &&
      editorX < this.editorGridBounds.width &&
      editorY < this.editorGridBounds.height
    ) {
      // Calculate grid location
      this.editorGridLocation = {
        x: Math.floor(editorX / this.renderConfig.TileSize),
        y: Math.floor(editorY / this.renderConfig.TileSize),
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
          this.placeRectangle.setPosition(
            this.editorGridBounds.x +
              this.editorGridLocation.x * this.renderConfig.TileSize,
            this.editorGridBounds.y +
              this.editorGridLocation.y * this.renderConfig.TileSize
          );
          if (this.placeTileOverlay) {
            this.placeTileOverlay.setPosition(
              this.editorGridBounds.x +
                this.editorGridLocation.x * this.renderConfig.TileSize,
              this.editorGridBounds.y +
                this.editorGridLocation.y * this.renderConfig.TileSize
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
    this.placeRectangle.setInteractive();
    this.placeRectangle.on("pointerdown", () => this.placeObject());

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

    this.loadingText = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading Level Editor",
      {
        font: "32px Droid Sans Mono",
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
