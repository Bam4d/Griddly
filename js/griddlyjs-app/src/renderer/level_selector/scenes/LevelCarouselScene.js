import Phaser from "phaser";

import Block2DRenderer from "../../Block2DRenderer";
import Sprite2DRenderer from "../../Sprite2DRenderer";
import {
  COLOR_LOADING_TEXT,
  COLOR_PANEL_LIGHT,
  COLOR_SELECT_TILE_HIGHLIGHTED,
  COLOR_PANEL_DARK,
  COLOR_FOREGROUND,
  COLOR_PANEL_LIGHTER,
} from "../../ThemeConsts";

const levelContainerHeight = 100;
const levelContainerWidth = 100;

const levelContainerInternalPadding = 5;

const levelContainerPaddingX = 10;
const levelContainerPaddingY = 10;

const levelContainerSliderPaddingX = 5;
const levelContainerSliderPaddingY = 5;
const levelContainerSliderHeight = 20;

const levelContainerSliderBarHeight = 14;
const levelContainerSliderBarPaddingX = 3;
const levelContainerSliderBarPaddingY = 3;

class LevelCarouselScene extends Phaser.Scene {
  constructor() {
    super("PreviewScene");
  }

  init = (data) => {
    this.gdy = data.gdy;

    this.onDisplayMessage = data.onDisplayMessage;
    this.editorStateHandlers = data.editorStateHandlers;
    this.avatarObject = this.gdy.Environment.Player.AvatarObject;
    this.renderConfig = data.rendererConfig;
    this.rendererName = data.rendererName;
    this.selectedLevelId = data.selectedLevelId;

    this.onSelectLevel = data.onSelectLevel;

    this.carouselState = {
      levels: [],
    };

    for (const [levelId, stateHandler] of this.editorStateHandlers) {
      const levelState = stateHandler.getState();

      // We want the level to fit in the selection box
      const scaledTileSize =
        (levelContainerHeight - levelContainerInternalPadding * 2) /
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

      grenderer.addObject(
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

  highlightSelectedLevel = (levelId) => {
    this.carouselState.levels.forEach((carouselItem) => {
      const selectLevelBg = carouselItem.selectLevelBg;
      if (levelId === carouselItem.levelId) {
        selectLevelBg.setStrokeStyle(3, COLOR_SELECT_TILE_HIGHLIGHTED);
      } else {
        selectLevelBg.setStrokeStyle(0);
      }
    });
  };

  selectLevel = (levelId) => {
    this.highlightSelectedLevel(levelId);
    this.onSelectLevel(levelId);
  };

  updateCarousel = () => {
    const selectLevelContainers = [];
    this.carouselState.levels.forEach((carouselItem, idx) => {
      const selectLevelX =
        levelContainerPaddingX +
        (levelContainerWidth + 2 * levelContainerPaddingX) *
          carouselItem.levelId;

      const selectLevelContainer = this.add.container(
        selectLevelX + levelContainerWidth / 2,
        levelContainerPaddingY + levelContainerHeight / 2
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
        COLOR_PANEL_DARK
      );

      selectLevelBg.setDepth(-10);
      selectLevelBg.setInteractive();
      selectLevelBg.on("pointerdown", () =>
        this.selectLevel(carouselItem.levelId)
      );

      this.carouselState.levels[idx].selectLevelBg = selectLevelBg;

      const levelIdText = this.add.text(
        -levelContainerWidth/2,
        -levelContainerHeight/2,
        ""+idx,
        {
          fontFamily: "Droid Sans Mono",
          fill: COLOR_LOADING_TEXT,
          align: "center",
        }
      );
      levelIdText.setDepth(10);
      levelIdText.setOrigin(0,0);

      selectLevelContainer.add(levelIdText);
      selectLevelContainer.add(selectLevelBg);

      selectLevelContainer.sort("depth");
      selectLevelContainers.push({
        origX: selectLevelContainer.x,
        container: selectLevelContainer,
      });
    });

    this.highlightSelectedLevel(this.selectedLevelId);

    // Slider bar for carousel
    const levelContainerSliderWidth =
      this.cameras.main.width - 2 * levelContainerSliderPaddingX;

    const sliderBarContainer = this.add.rectangle(
      levelContainerSliderPaddingX,
      2 * levelContainerPaddingY +
        levelContainerHeight +
        levelContainerSliderPaddingY,
      levelContainerSliderWidth,
      levelContainerSliderHeight,
      COLOR_PANEL_DARK
    );
    sliderBarContainer.setOrigin(0, 0);
    sliderBarContainer.setInteractive();

    // Calculate width of the slider bar
    const totalItemsLength =
      this.carouselState.levels.length *
      (levelContainerWidth + 2 * levelContainerPaddingX);
    const sliderBarWidthRatio = levelContainerSliderWidth / totalItemsLength;
    const sliderBarWidth =
      sliderBarWidthRatio * levelContainerSliderWidth -
      2 * levelContainerSliderBarPaddingX;

    const sliderBarMaxX = levelContainerSliderWidth - sliderBarWidth;
    const sliderBarMinX =
      levelContainerSliderBarPaddingX + levelContainerSliderPaddingX;

    const sliderBar = this.add.rectangle(
      levelContainerSliderBarPaddingX,
      2 * levelContainerPaddingY +
        levelContainerHeight +
        levelContainerSliderPaddingY +
        levelContainerSliderBarPaddingX,
      sliderBarWidth,
      levelContainerSliderBarHeight,
      COLOR_PANEL_LIGHT
    );
    sliderBar.setOrigin(0, 0);
    sliderBar.setInteractive();

    const moveSliderToLocation = function (sliderLocation) {
      sliderBar.setX(
        Math.min(sliderBarMaxX, Math.max(sliderBarMinX, sliderLocation))
      );

      const containerXOffset =
        ((totalItemsLength -
          (levelContainerSliderWidth - 2 * levelContainerPaddingX)) *
          (sliderBar.x - sliderBarMinX)) /
        (levelContainerSliderWidth - sliderBarWidth);

      selectLevelContainers.forEach((levelContainer) => {
        levelContainer.container.x = levelContainer.origX - containerXOffset;
      });
    };

    const sliderLocation =
      (levelContainerSliderWidth *
        (this.selectedLevelId *
          (levelContainerWidth + 2 * levelContainerPaddingX))) /
      totalItemsLength;

    console.log(this.selectedLevelId);
    console.log(sliderLocation);
    moveSliderToLocation(sliderLocation);

    // Mouse events
    sliderBar.on("pointerover", () => {
      sliderBar.setFillStyle(COLOR_PANEL_LIGHTER);
    });
    sliderBar.on("pointerout", () => {
      sliderBar.setFillStyle(COLOR_PANEL_LIGHT);
    });
    sliderBar.on("pointermove", (pointer) => {
      if (pointer.isDown) {
        const sliderLocation = pointer.position.x - sliderBarWidth / 2.0;
        moveSliderToLocation(sliderLocation);
      }
    });

    sliderBarContainer.on("pointerdown", (pointer) => {
      const sliderLocation = pointer.position.x - sliderBarWidth / 2.0;
      moveSliderToLocation(sliderLocation);
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
