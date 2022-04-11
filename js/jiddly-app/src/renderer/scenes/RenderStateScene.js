import Phaser from "phaser";
import Block2DRenderer from "./Block2DRenderer";
import Sprite2DRenderer from "./Sprite2DRenderer";

class RenderStateScene extends Phaser.Scene {
  constructor() {
    super("RenderStateScene");

    this.stateHash = 0;
    this.loaded = false;
    this.tileSize = 32;
  }

  init = (data) => {
    this.data = data;

    // Functions to interact with the environment
    this.getEnvState = this.data.getEnvState;
    this.envStep = this.data.envStep;
    this.envReset = this.data.envReset;

    // Data about the environment
    this.gdy = this.data.gdy;
    this.rendererName = this.data.rendererName;

    if(this.rendererName === "Block2D") {
      this.renderer = new Block2DRenderer(this, this.tileSize);
    } else if(this.rendererName === "Sprite2D") {
      this.renderer = new Sprite2DRenderer(this, this.tileSize);
    }

    this.renderData = {
      objects: {},
    };
  };

  displayError = (error) => {};

  updateState = (state) => {
    const newObjectIds = state.objects.map((object) => {
      return object.id;
    });

    state.objects.forEach((object) => {
      const objectTemplateName = object.name + object.renderTileId;
      if (object.id in this.renderData.objects) {
        const currentObjectData = this.renderData.objects[object.id];
        this.renderer.updateObject(
          currentObjectData.sprite,
          objectTemplateName,
          object.location.x + 0.5,
          object.location.y + 0.5,
          object.orientation
        );

        this.renderData.objects[object.id] = {
          ...currentObjectData,
          object,
        };
      } else {
        const sprite = this.renderer.addObject(
          objectTemplateName,
          object.location.x + 0.5,
          object.location.y + 0.5,
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
    this.keyboardMapping = this.input.keyboard.addKeys("W,A,S,D");
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
        this.envStep(action);
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
        font: "65px Arial",
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
  };

  update = () => {
    if (!this.loaded) {
      this.loadingText.setX(this.cameras.main.width / 2);
      this.loadingText.setY(this.cameras.main.height / 2);
      this.loadingText.setOrigin(0.5, 0.5);
    } else {
      const state = this.getEnvState();

      if (state.hash && this.stateHash !== state.hash) {
        console.log("Updating State:", state);
        this.stateHash = state.hash;
        this.updateState(state);
      }

      this.processUserAction();
    }
  };
}

export default RenderStateScene;
