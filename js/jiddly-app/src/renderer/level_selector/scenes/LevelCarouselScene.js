import Phaser from "phaser";

import Block2DRenderer from "../../Block2DRenderer";
import Sprite2DRenderer from "../../Sprite2DRenderer";
import {
  COLOR_LOADING_TEXT,
  COLOR_PANEL_LIGHT,
  COLOR_SELECT_TILE_HIGHLIGHTED,
} from "../../ThemeConsts";

const levelContainerHeight = 100;
const levelContainerWidth = 100;

const levelContainerInternalPadding = 5;

const levelContainerPaddingX = 10;

class LevelCarouselScene extends Phaser.Scene {
  constructor() {
    super("PreviewScene");

    this.editorCenterX = 0;
    this.editorCenterY = 0;

    this.origDragPoint = null;

    this.editorGridLocation = {
      x: 0,
      y: 0,
    };

    this.selectTileBgMap = {};

    this.carouselState = {
      levels: [],
    };
  }

  init = (data) => {
    this.gdy = data.gdy;

    this.onDisplayMessage = data.onDisplayMessage;
    this.editorStateHandlers = data.editorStateHandlers;
    this.avatarObject = this.gdy.Environment.Player.AvatarObject;
    this.renderConfig = data.rendererConfig;
    this.rendererName = data.rendererName;

    this.onSelectLevel = data.onSelectLevel;

    for (const [levelId, stateHandler] of this.editorStateHandlers) {
      const levelState = stateHandler.getState();

      // We want the level to fit in the selection box
      const scaledTileSize =
        (levelContainerHeight-levelContainerInternalPadding*2) /
        Math.max(levelState.gridWidth, levelState.gridHeight);

      const rendererConfig = {
        ...this.rendererConfig,
        TileSize: scaledTileSize,
      };

      let grenderer;

      if (this.renderConfig.Type === "BLOCK_2D") {
        grenderer = new Block2DRenderer(
          this,
          this.rendererName,
          rendererConfig,
          this.avatarObject,
          false
        );
      } else if (this.renderConfig.Type === "SPRITE_2D") {
        grenderer = new Sprite2DRenderer(
          this,
          this.rendererName,
          rendererConfig,
          this.avatarObject,
          false
        );
      }

      this.carouselState.levels.push({
        levelId: levelId,
        state: levelState,
        grenderer: grenderer,
      });
    }
  };

  updateState = (carouselItem) => {
    const objectList = [];

    const state = carouselItem.state;
    const grenderer = carouselItem.grenderer;

    for (const objectsAtLocation in state.objects) {
      for (const object in state.objects[objectsAtLocation]) {
        objectList.push(state.objects[objectsAtLocation][object]);
      }
    }

    grenderer.beginUpdate(objectList, state);

    objectList.forEach((object) => {
      const objectTemplateName = object.name + object.renderTileId;

      const sprite = grenderer.addObject(
        object.name,
        objectTemplateName,
        object.location.x,
        object.location.y,
        object.orientation
      );
    });
  };

  preload() {
    console.log("Preview Scene - Preload");

    this.input.mouse.disableContextMenu();

    this.loadingText = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading Levels",
      {
        fontFamily: "Droid Sans Mono",
        fill: COLOR_LOADING_TEXT,
        align: "center",
      }
    );

    this.loadingText.setX(this.cameras.main.width / 2);
    this.loadingText.setY(this.cameras.main.height / 2);
    this.loadingText.setOrigin(0.5, 0.5);

    this.carouselState.levels.forEach((carouselItem) => {
      carouselItem.grenderer.loadTemplates(this.gdy.Objects);
    });
  }

  selectLevel = (levelId) => {
    console.log("Level selected", levelId);

    this.carouselState.levels.forEach((carouselItem) => {
      const selectLevelBg = carouselItem.selectLevelBg;
      if (levelId === carouselItem.levelId) {
        selectLevelBg.setStrokeStyle(3, COLOR_SELECT_TILE_HIGHLIGHTED);
      } else {
        selectLevelBg.setStrokeStyle(0);
      }
    });

    this.onSelectLevel(levelId);
  };

  updateCarousel = () => {
    this.carouselState.levels.forEach((carouselItem, idx) => {
      const selectLevelX =
        levelContainerPaddingX +
        (levelContainerWidth + 2 * levelContainerPaddingX) *
          carouselItem.levelId;

      const selectLevelContainer = this.add.container(
        selectLevelX + levelContainerWidth / 2,
        10 + levelContainerHeight / 2
      );
      const levelState = carouselItem.state;

      carouselItem.grenderer.init(
        levelState.gridWidth,
        levelState.gridHeight,
        selectLevelContainer
      );

      this.updateState(carouselItem);

      const selectLevelBg = this.add.rectangle(
        0,
        0,
        levelContainerWidth,
        levelContainerHeight,
        COLOR_PANEL_LIGHT
      );

      selectLevelBg.setDepth(-10);
      selectLevelBg.setInteractive();
      selectLevelBg.on("pointerdown", () =>
        this.selectLevel(carouselItem.levelId)
      );

      this.carouselState.levels[idx].selectLevelBg = selectLevelBg;

      selectLevelContainer.add(selectLevelBg);

      selectLevelContainer.sort("depth");
    });
  };

  create() {
    console.log("Preview Scene - Create");

    this.loadingText.destroy();
    this.loaded = true;

    this.updateCarousel();
  }

  update() {
    if (!this.loaded) {
      this.loadingText.setX(this.cameras.main.width / 2);
      this.loadingText.setY(this.cameras.main.height / 2);
      this.loadingText.setOrigin(0.5, 0.5);
    } else {
      // if (this.grenderer) {
      //   const state = this.editorStateHandler.getState();
      //   if (state && this.stateHash !== state.hash) {
      //     this.stateHash = state.hash;
      //     if(state.hash === 0) {
      //       this.grenderer.recenter(state.gridWidth, state.gridHeight);
      //     }
      //     this.updateState(state);
      //   }
      // }
    }
  }
}

export default LevelCarouselScene;
