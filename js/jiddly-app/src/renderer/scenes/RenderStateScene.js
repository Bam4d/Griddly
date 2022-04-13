import Phaser from "phaser";
import Block2DRenderer from "./Block2DRenderer";
import Sprite2DRenderer from "./Sprite2DRenderer";

class RenderStateScene extends Phaser.Scene {
  constructor() {
    super("RenderStateScene");

    this.stateHash = 0;
    this.loaded = false;
    this.defaultTileSize = 24;
  }

  getRendererConfig = (rendererName) => {
    let rendererConfig = {};
    const observers = this.gdy.Environment.Observers;
    if (rendererName in observers) {
      rendererConfig = observers[rendererName];
    }

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

    return rendererConfig;
  };

  init = (data) => {
    this.data = data;

    // Functions to interact with the environment
    this.jiddly = this.data.jiddly;

    // Data about the environment
    this.gdy = this.data.gdy;

    this.gridHeight = this.jiddly.getHeight();
    this.gridWidth = this.jiddly.getWidth();

    this.rendererName = this.data.rendererName;

    this.renderConfig = this.getRendererConfig(this.rendererName);
    this.avatarObject = this.gdy.Environment.Player.AvatarObject;

    if (this.renderConfig.Type === "BLOCK_2D") {
      this.renderer = new Block2DRenderer(
        this,
        this.renderConfig,
        this.avatarObject
      );
    } else if (this.renderConfig.Type === "SPRITE_2D") {
      this.renderer = new Sprite2DRenderer(
        this,
        this.renderConfig,
        this.avatarObject
      );
    }

    this.renderData = {
      objects: {},
    };
  };

  displayError = (error) => {
    console.log("Display Error: ", error);
  };

  updateState = (state) => {
    const newObjectIds = state.objects.map((object) => {
      return object.id;
    });

    this.renderer.beginUpdate(state.objects);

    state.objects.forEach((object) => {
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
  };

  setupKeyboardMapping = () => {
    // TODO: Use action input mapping to set this up.
    this.keyboardMapping = this.input.keyboard.addKeys("W,A,S,D", false);

    // When the mouse leaves the window we stop collecting keys
    this.input.on(Phaser.Input.Events.POINTER_DOWN_OUTSIDE, () => {
      this.input.keyboard.enabled = false;
    });

    // When we click back in the scene we collect keys
    this.input.on(Phaser.Input.Events.POINTER_DOWN, () => {
      document.activeElement.blur();
      this.input.keyboard.enabled = true;
    });
  };

  processUserAction = () => {
    if (!this.cooldown) {
      this.cooldown = true;
      setTimeout(() => {
        this.cooldown = false;
      }, 100);

      // TODO: actually use the descriptions from the action input mapping
      let action = -1;
      if (this.keyboardMapping["W"].isDown) {
        action = 2;
      } else if (this.keyboardMapping["A"].isDown) {
        action = 1;
      } else if (this.keyboardMapping["S"].isDown) {
        action = 4;
      } else if (this.keyboardMapping["D"].isDown) {
        action = 3;
      }

      if (action >= 0) {
        const stepResult = this.jiddly.step(action);
        console.log("Step Result", stepResult);

        if (stepResult.terminated) {
          this.jiddly.reset();
        }
      }
    }
  };

  preload = () => {
    const envName = this.gdy.Environment.Name;

    this.loadingText = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading assets for " + envName,
      {
        font: "32px Arial",
        fill: "#ff0044",
        align: "center",
      }
    );

    this.loadingText.setX(this.cameras.main.width / 2);
    this.loadingText.setY(this.cameras.main.height / 2);
    this.loadingText.setOrigin(0.5, 0.5);

    this.renderer.loadTemplates(this.gdy.Objects);
  };

  create = () => {
    console.log("Create");

    this.loadingText.destroy();
    this.loaded = true;

    this.mapping = this.setupKeyboardMapping();
    this.renderer.init(this.gridWidth, this.gridHeight);
  };

  update = () => {
    if (!this.loaded) {
      this.loadingText.setX(this.cameras.main.width / 2);
      this.loadingText.setY(this.cameras.main.height / 2);
      this.loadingText.setOrigin(0.5, 0.5);
    } else {
      const state = this.jiddly.getState();

      this.updateState(state);

      this.processUserAction();
    }
  };
}

export default RenderStateScene;
