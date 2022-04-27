import Phaser from "phaser";

import EditorStateHandler from "../EditorStateHandler";
import Block2DRenderer from "../../Block2DRenderer";
import Sprite2DRenderer from "../../Sprite2DRenderer";

const COLOR_SELECT_TILE_TEXT = "#3dc9b0";

const COLOR_LOADING = 0x3dc9b0;
const COLOR_ERROR = 0xf14c4c;

const COLOR_SELECT = 0x3dc9b0;
const COLOR_PLACE = 0xce9178;

const COLOR_FOREGROUND = 0xd4d4d4;
const COLOR_PANEL_DARK = 0x1e1e1e;
const COLOR_PANEL_LIGHT = 0x303030;

const selectTileBoxHeight = 30;
const selectTileBoxPadding = 3;

const selectTileSpriteOffset = 3;

const toolBoxOffset = 100;

class EditorScene extends Phaser.Scene {
  constructor() {
    super("EditorScene");
  }

  createTileMenu() {

    // Make a rectangle on the left and add more rectangles to it
    this.selectTilePanel = this.add.rectangle(
      0,
      0,
      this.cameras.main.width / 5,
      this.cameras.main.height,
      COLOR_PANEL_DARK,
    );

    this.selectTilePanel.setOrigin(0, 0);
    let o = 0;
    for(const objectTemplateName in this.renderer.objectTemplates) {
      const selectTileBg = this.add.rectangle(
        this.selectTilePanel.x + selectTileBoxPadding, 
        toolBoxOffset + this.selectTilePanel.y + o*(selectTileBoxPadding*2+selectTileBoxHeight),
        this.selectTilePanel.width-3*selectTileBoxPadding,
        selectTileBoxHeight,
        COLOR_PANEL_LIGHT,
      );
      selectTileBg.setOrigin(0, 0);

      const selectTileTopRight = selectTileBg.getTopRight();
      const selectTileTopLeft = selectTileBg.getTopLeft();

      // Create a sprite and add it to the list .. if there are multiple tiles we can cycle through them on mouseover
      const objectTemplate = this.renderer.objectTemplates[objectTemplateName];
      const selectTileText = this.add.text(
        selectTileTopLeft.x+selectTileSpriteOffset,
        selectTileTopLeft.y+selectTileSpriteOffset,
        objectTemplate.name,
        {
          fontFamily: "Droid Sans Mono",
          color: COLOR_SELECT_TILE_TEXT,
          font: "16px"
        }

      );
      selectTileText.setOrigin(0,0);
      const selectTileSprite = this.add.sprite(
        selectTileTopRight.x-this.renderConfig.TileSize-selectTileSpriteOffset,
        selectTileTopRight.y+selectTileSpriteOffset,
        objectTemplate.id
      );
      selectTileSprite.setOrigin(0,0);


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
      height: (this.renderer.gridHeight + 7) * this.renderConfig.TileSize
    };
  };

  selectTool = (toolName) => {
    this.currentTool = toolName;
    switch (this.currentTool) {
      case "select":
        this.placeRectangle.setVisible(false);
        break;
      case "place":
        this.selectRectangle.setVisible(false);
        break;
      default:
        break;
    }
  };

  mouseMoved = (x, y) => {
    console.log(x, y);

    const editorX = x - this.editorGridBounds.x;
    const editorY = y - this.editorGridBounds.y;
    
    if(editorX>=0 && editorY>=0 && editorX<this.editorGridBounds.width && editorY<this.editorGridBounds.height) {
      // Calculate grid location
      this.editorGridLocation = {
        x: Math.floor(editorX / this.renderConfig.TileSize),
        y: Math.floor(editorY / this.renderConfig.TileSize),
      };

      switch (this.currentTool) {
        case "select":
          this.selectRectangle.setPosition(
            this.editorGridBounds.x +
              this.editorGridLocation.x * this.renderConfig.TileSize,
            this.editorGridBounds.y +
              this.editorGridLocation.y * this.renderConfig.TileSize
          );
          this.selectRectangle.setOrigin(0, 0);
          break;
        case "place":
          this.placeRectangle.setPosition(
            this.editorGridLocation.x * this.renderConfig.TileSize,
            this.editorGridLocation.y * this.renderConfig.TileSize
          );
          this.placeRectangle.setOrigin(0, 0);
          break;
        default:
          break;
      }

      console.log(this.editorGridLocation);
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
    this.selectRectangle.setStrokeStyle(1, COLOR_SELECT, 0.5);
    this.selectRectangle.setDepth(100);
    this.placeRectangle = this.add.rectangle(
      0,
      0,
      this.renderConfig.TileSize,
      this.renderConfig.TileSize
    );
    this.placeRectangle.setStrokeStyle(1, COLOR_PLACE, 0.5);
    this.placeRectangle.setDepth(100);

    this.placeRectangle.setVisible(false);

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
  };

  preload() {
    console.log("Editor Scene - Preload");

    this.loadingText = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading Level Editor",
      {
        font: "32px Droid Sans Mono",
        fill: COLOR_LOADING,
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

    if (this.renderer) {
      const state = this.editorStateHandler.getState();
      this.renderer.init(state.gridWidth, state.gridHeight);
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
          this.updateState(state);
        }
      }
    }
  }
}

export default EditorScene;
