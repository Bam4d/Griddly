import Phaser from "phaser";
import Block2DRenderer from "../../Block2DRenderer";
import Sprite2DRenderer from "../../Sprite2DRenderer";
import {
  COLOR_LOADING_TEXT,
  COLOR_PANEL_DARK,
  COLOR_PANEL_LIGHT,
  COLOR_PANEL_LIGHTER,
  COLOR_POLICY_DEBUG_PLAY_TEXT,
  COLOR_POLICY_DEBUG_STOP_TEXT,
  COLOR_POLICY_DEBUG_DISABLED_TEXT,
} from "../../ThemeConsts";

import * as tf from "@tensorflow/tfjs";

const policyMenuHeight = 45;

const debugButtonsOffsetX = 30;

const debugButtonsPaddingY = 5;

const debugButtonWidth = 35;
const debugButtonHeight = 35;

class PolicyDebuggerScene extends Phaser.Scene {
  constructor() {
    super("PolicyDebuggerScene");

    this.stateHash = 0;
    this.loaded = false;
    this.defaultTileSize = 24;
  }

  createPolicyMenu = () => {
    const policyMenu = this.add.rectangle(
      0,
      0,
      this.cameras.main.width,
      policyMenuHeight,
      COLOR_PANEL_DARK
    );
    policyMenu.setOrigin(0, 0);

    const debugButtonY = debugButtonsPaddingY + debugButtonHeight / 2;

    const playPolicyButtonX = debugButtonsOffsetX + debugButtonWidth / 2;
    const stopButtonX =
      this.cameras.main.width - debugButtonsOffsetX - debugButtonWidth;

    // Play Policy Button
    this.playPolicyBg = this.add.rectangle(
      playPolicyButtonX,
      debugButtonY,
      debugButtonWidth,
      debugButtonHeight,
      COLOR_PANEL_LIGHT
    );
    this.playPolicyBg.setDepth(10);
    this.playPolicyBg.setInteractive();
    this.playPolicyBg.on("pointerdown", () => this.beginPolicy());
    this.playPolicyText = this.add
      .text(playPolicyButtonX, debugButtonY, "\uf04b", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_POLICY_DEBUG_PLAY_TEXT,
        fontSize: "24px",
      })
      .setDepth(11)
      .setOrigin(0.5, 0.5);

    // Stop Button
    this.stopBg = this.add.rectangle(
      stopButtonX,
      debugButtonY,
      debugButtonWidth,
      debugButtonHeight,
      COLOR_PANEL_LIGHT
    );
    this.stopBg.setDepth(10);
    this.stopBg.setInteractive();
    this.stopBg.on("pointerdown", () => this.stopPolicyPlayback());
    this.stopText = this.add
      .text(stopButtonX, debugButtonY, "\uf04d", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_POLICY_DEBUG_STOP_TEXT,
        fontSize: "24px",
      })
      .setDepth(11)
      .setOrigin(0.5, 0.5);
  };

  updatePolicyMenu = () => {
    if (this.isRunningPolicy) {
      this.playPolicyText.setColor(COLOR_POLICY_DEBUG_DISABLED_TEXT);
      this.playPolicyBg.setFillStyle(COLOR_PANEL_LIGHTER);

      this.stopBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.stopText.setColor(COLOR_POLICY_DEBUG_STOP_TEXT);
    } else {
      this.playPolicyText.setColor(COLOR_POLICY_DEBUG_PLAY_TEXT);
      this.playPolicyBg.setFillStyle(COLOR_PANEL_DARK);

      this.stopBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.stopText.setColor(COLOR_POLICY_DEBUG_DISABLED_TEXT);
    }
  };

  createModals = () => {
    // Set the modals to invisible
    this.variableDebugModalActive = false;

    // Get all the global variables
    this.globalVariableDebugText = this.getGlobalVariableDebugText();

    this.variableDebugModal = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 5,
      this.globalVariableDebugText
    );
    this.variableDebugModal.setBackgroundColor("#000000AA");
    this.variableDebugModal.setDepth(100);
    this.variableDebugModal.setOrigin(0, 0);
    this.variableDebugModal.setVisible(false);
  };

  init = (data) => {
    try {
      // Functions to interact with the environment
      this.jiddly = data.jiddly;

      // Data about the environment
      this.gdy = data.gdy;
      this.onDisplayMessage = data.onDisplayMessage;
      this.onPolicyComplete = data.onPolicyComplete;

      this.gridHeight = this.jiddly.getHeight();
      this.gridWidth = this.jiddly.getWidth();

      this.avatarObject = this.gdy.Environment.Player.AvatarObject;
      this.rendererName = data.rendererName;
      this.renderConfig = data.rendererConfig;

      if (this.renderConfig.Type === "BLOCK_2D") {
        this.grenderer = new Block2DRenderer(
          this,
          this.rendererName,
          this.renderConfig,
          this.avatarObject
        );
      } else if (this.renderConfig.Type === "SPRITE_2D") {
        this.grenderer = new Sprite2DRenderer(
          this,
          this.rendererName,
          this.renderConfig,
          this.avatarObject
        );
      }
    } catch (e) {
      this.displayError("Cannot load GDY file.", e);
    }

    this.renderData = {
      objects: {},
    };

    this.player = this.jiddly.players[0];
    this.playerObsSpace = this.player.getObservationDescription();

    this.model = data.model;
  };

  displayError = (message, error) => {
    this.onDisplayMessage(message, "error", error);
  };

  displayWarning = (message, error) => {
    this.onDisplayMessage(message, "warning", error);
  };

  updateState = (state) => {
    const newObjectIds = state.objects.map((object) => {
      return object.id;
    });

    this.grenderer.recenter(this.jiddly.getWidth(), this.jiddly.getHeight());

    this.grenderer.beginUpdate(state.objects);

    state.objects.forEach((object) => {
      const objectTemplateName = object.name + object.renderTileId;
      if (object.id in this.renderData.objects) {
        const currentObjectData = this.renderData.objects[object.id];
        this.grenderer.updateObject(
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
        const sprite = this.grenderer.addObject(
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

  toMovementKey(vector) {
    return `${vector.x},${vector.y}`;
  }

  getGlobalVariableDebugText() {
    const globalVariables = this.jiddly.getGlobalVariables();

    const globalVariableDescription = [];
    const playerVariableDescription = [];
    for (const variableName in globalVariables) {
      const variableData = globalVariables[variableName];
      if (Object.keys(variableData).length === 1) {
        // We have a global variable
        const variableValue = variableData[0];
        globalVariableDescription.push(variableName + ": " + variableValue);
      } else {
        // We have a player variable
        if (this.jiddly.playerCount === 1) {
          const variableValue = variableData[1];
          playerVariableDescription.push(variableName + ": " + variableValue);
        } else {
          let variableValues = "";
          for (let p = 0; p < this.jiddly.playerCount; p++) {
            const variableValue = variableData[p + 1];
            variableValues += "\t" + (p + 1) + ": " + variableValue;
          }

          playerVariableDescription.push(variableName + ":" + variableValues);
        }
      }
    }

    return [
      "Global Variables:",
      ...globalVariableDescription,
      "",
      "Player Variables:",
      ...playerVariableDescription,
    ];
  }

  updateModals() {
    if (this.variableDebugModalActive) {
      this.variableDebugModal.setText(this.globalVariableDebugText);
      this.variableDebugModal.setFontFamily("Droid Sans Mono");
      this.variableDebugModal.setPosition(0, 0);
      this.variableDebugModal.setWordWrapWidth(this.cameras.main.width / 2);
    }
  }

  toggleVariableDebugModal() {
    this.variableDebugModalActive = !this.variableDebugModalActive;
    this.variableDebugModal.setVisible(this.variableDebugModalActive);
  }

  setupFlatActionMap = () => {
    const actionInputMappings = this.jiddly.getActionInputMappings();
    const actionNames = this.jiddly.getActionNames();

    const flatActionMap = [];

    actionNames.forEach((actionName, actionTypeId) => {
      const actionMapping = actionInputMappings[actionName];
      if (!actionMapping.internal) {
        const inputMappings = Object.entries(actionMapping.inputMappings);
        console.log(inputMappings);

        inputMappings.forEach((inputMapping) => {
          const actionId = Number(inputMapping[0]);

          flatActionMap.push([actionTypeId, actionId]);
        });
      }
    });

    return flatActionMap;
  };

  setupKeyboardMapping = () => {
    this.input.keyboard.on("keydown-I", (event) => {
      this.toggleVariableDebugModal();
    });

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

  stopPolicyPlayback = () => {
    if (this.isRunningPolicy) {
      this.endPolicy();
    }

    this.resetLevel();
  };

  beginPolicy = () => {
    this.PolicyActionIdx = 0;
    this.isRunningPolicy = true;
  };

  endPolicy = () => {
    this.PolicyActionIdx = 0;
    this.isRunningPolicy = false;
  };

  resetLevel = () => {
    this.jiddly.reset();
  };

  processPolicy = () => {
    if (!this.cooldown) {
      this.cooldown = true;
      setTimeout(() => {
        this.cooldown = false;
      }, 10);

      try {
        const shape = this.playerObsSpace.Shape;

        // Generates the actions thanks to the agent's policy model
        if (this.model != null) {
          const state = this.player.observe();

          const singletonState = tf.reshape(state, [1, ...shape]);

          const inputs = {
            input_0: singletonState,
          };

          // let output = 'main/mul:0'
          const output = "output_0";

          const logits = this.model.execute(inputs, output).arraySync()[0];

          const action = tf.multinomial(logits, 1).arraySync()[0];

          const stepResult = this.jiddly.step(this.flatActionMap[action]);

          if (stepResult.reward > 0) {
            console.log("Reward: ", stepResult.reward);
          }

          if (stepResult.terminated) {
            this.jiddly.reset();
            this.endPolicy();
          }
        }
      } catch (e) {
        this.onDisplayMessage(
          "Loaded model is incompatible with the environment.",
          "error",
          e
        );
        this.stopPolicyPlayback();
      }

      return this.jiddly.getState();
    }
  };

  processUserAction = () => {
    if (!this.cooldown) {
      this.cooldown = true;
      setTimeout(() => {
        this.cooldown = false;
      }, 100);

      return this.jiddly.getState();
    }
  };

  preload = async () => {
    const envName = this.gdy.Environment.Name;

    this.input.mouse.disableContextMenu();

    this.loadingText = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading assets for " + envName,
      {
        fontFamily: "Droid Sans Mono",
        font: "32px",
        fill: COLOR_LOADING_TEXT,
        align: "center",
      }
    );

    this.loadingText.setX(this.cameras.main.width / 2);
    this.loadingText.setY(this.cameras.main.height / 2);
    this.loadingText.setOrigin(0.5, 0.5);
    if (this.grenderer) {
      this.grenderer.loadTemplates(this.gdy.Objects);
    }
  };

  create = () => {
    console.log("Create");

    this.loadingText.destroy();
    this.loaded = true;

    if (this.grenderer) {
      this.flatActionMap = this.setupFlatActionMap();
      this.mapping = this.setupKeyboardMapping();
      this.grenderer.init(this.gridWidth, this.gridHeight);
      this.updateState(this.jiddly.getState());
      this.createModals();
      this.updateModals();
      this.createPolicyMenu();
      this.updatePolicyMenu();
    }
  };

  update = () => {
    if (!this.loaded) {
      this.loadingText.setX(this.cameras.main.width / 2);
      this.loadingText.setY(this.cameras.main.height / 2);
      this.loadingText.setOrigin(0.5, 0.5);
    } else {
      if (this.grenderer) {
        let state;
        if (this.isRunningPolicy) {
          state = this.processPolicy();
        } else {
          state = this.processUserAction();
        }

        if (state && this.stateHash !== state.hash) {
          this.stateHash = state.hash;
          this.updateState(state);
        }

        this.updateModals();
        this.updatePolicyMenu();
      }
    }
  };
}

export default PolicyDebuggerScene;
