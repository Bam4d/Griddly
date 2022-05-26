import Phaser from "phaser";
import Block2DRenderer from "../../Block2DRenderer";
import Sprite2DRenderer from "../../Sprite2DRenderer";
import {
  COLOR_LOADING_TEXT,
  COLOR_PANEL_DARK,
  COLOR_PANEL_LIGHT,
  COLOR_PANEL_LIGHTER,
  COLOR_RECORDING_RECORD_TEXT,
  COLOR_RECORDING_PLAY_TEXT,
  COLOR_RECORDING_STOP_TEXT,
  COLOR_RECORDING_DISABLED_TEXT,
} from "../../ThemeConsts";

const trajectoryMenuHeight = 45;

const recordingButtonsOffsetX = 30;

const recordingButtonPaddingY = 5;
const recordingButtonPaddingX = 5;

const recordingButtonWidth = 35;
const recordingButtonHeight = 35;

class HumanPlayerScene extends Phaser.Scene {
  constructor() {
    super("HumanPlayerScene");

    this.stateHash = 0;
    this.loaded = false;
    this.defaultTileSize = 24;
  }

  createTrajectoryMenu = () => {
    const trajectoryMenu = this.add.rectangle(
      0,
      0,
      this.cameras.main.width,
      trajectoryMenuHeight,
      COLOR_PANEL_DARK
    );
    trajectoryMenu.setOrigin(0, 0);

    const recordingButtonY =
      recordingButtonPaddingY + recordingButtonHeight / 2;
    const startRecordingButtonX =
      recordingButtonsOffsetX + recordingButtonWidth / 2;
    const playTrajectoryButtonX =
      recordingButtonsOffsetX +
      2 * recordingButtonPaddingX +
      recordingButtonWidth +
      recordingButtonWidth / 2;
    const stopButtonX =
      this.cameras.main.width - recordingButtonsOffsetX - recordingButtonWidth;

    // Start Recording Button
    this.startRecordingBg = this.add.rectangle(
      startRecordingButtonX,
      recordingButtonY,
      recordingButtonWidth,
      recordingButtonHeight,
      COLOR_PANEL_LIGHT
    );
    // this.startRecordingBg.setOrigin(0,0);
    this.startRecordingBg.setDepth(10);
    this.startRecordingBg.setInteractive();
    this.startRecordingBg.on("pointerdown", () => this.beginRecording());
    this.startRecordingText = this.add
      .text(startRecordingButtonX, recordingButtonY, "\uf03d", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_RECORDING_RECORD_TEXT,
        fontSize: "24px",
      })
      .setDepth(11)
      .setOrigin(0.5, 0.5);

    // Play Trajectory Button
    this.playTrajectoryBg = this.add.rectangle(
      playTrajectoryButtonX,
      recordingButtonY,
      recordingButtonWidth,
      recordingButtonHeight,
      COLOR_PANEL_LIGHT
    );
    this.playTrajectoryBg.setDepth(10);
    this.playTrajectoryBg.setInteractive();
    this.playTrajectoryBg.on("pointerdown", () => this.beginPlayback());
    this.playTrajectoryText = this.add
      .text(playTrajectoryButtonX, recordingButtonY, "\uf04b", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_RECORDING_PLAY_TEXT,
        fontSize: "24px",
      })
      .setDepth(11)
      .setOrigin(0.5, 0.5);

    // Stop Button
    this.stopBg = this.add.rectangle(
      stopButtonX,
      recordingButtonY,
      recordingButtonWidth,
      recordingButtonHeight,
      COLOR_PANEL_LIGHT
    );
    this.stopBg.setDepth(10);
    this.stopBg.setInteractive();
    this.stopBg.on("pointerdown", () => this.stopRecordingOrPlayback());
    this.stopText = this.add
      .text(stopButtonX, recordingButtonY, "\uf04d", {
        fontFamily: "Font Awesome Solid",
        color: COLOR_RECORDING_STOP_TEXT,
        fontSize: "24px",
      })
      .setDepth(11)
      .setOrigin(0.5, 0.5);
  };

  updateTrajectoryMenu = () => {
    const hasTrajectories = this.currentTrajectoryBuffer ? true : false;
    if (this.isRecordingTrajectory) {
      this.startRecordingBg.setFillStyle(COLOR_PANEL_LIGHTER);
      this.startRecordingText.setColor(COLOR_RECORDING_RECORD_TEXT);

      this.playTrajectoryText.setColor(COLOR_RECORDING_DISABLED_TEXT);
      this.playTrajectoryBg.setFillStyle(COLOR_PANEL_DARK);

      this.stopBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.stopText.setColor(COLOR_RECORDING_STOP_TEXT);
    } else if (this.isRunningTrajectory) {
      this.startRecordingBg.setFillStyle(COLOR_PANEL_DARK);
      this.startRecordingText.setColor(COLOR_RECORDING_DISABLED_TEXT);

      this.playTrajectoryText.setColor(COLOR_RECORDING_PLAY_TEXT);
      this.playTrajectoryBg.setFillStyle(COLOR_PANEL_LIGHTER);

      this.stopBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.stopText.setColor(COLOR_RECORDING_STOP_TEXT);
    } else if (hasTrajectories) {
      this.startRecordingBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.startRecordingText.setColor(COLOR_RECORDING_RECORD_TEXT);

      this.playTrajectoryText.setColor(COLOR_RECORDING_PLAY_TEXT);
      this.playTrajectoryBg.setFillStyle(COLOR_PANEL_LIGHT);

      this.stopBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.stopText.setColor(COLOR_RECORDING_DISABLED_TEXT);
    } else {
      this.startRecordingBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.startRecordingText.setColor(COLOR_RECORDING_RECORD_TEXT);

      this.playTrajectoryText.setColor(COLOR_RECORDING_DISABLED_TEXT);
      this.playTrajectoryBg.setFillStyle(COLOR_PANEL_DARK);

      this.stopBg.setFillStyle(COLOR_PANEL_LIGHT);
      this.stopText.setColor(COLOR_RECORDING_DISABLED_TEXT);
    }
  };

  createModals = () => {
    // Set the modals to invisible
    this.variableDebugModalActive = false;
    this.controlsModalActive = false;

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

    const actionDescription = [];
    const actionNames = this.jiddly.getActionNames();
    actionNames.forEach((actionName) => {
      actionDescription.push(actionName + ": ");
      this.keyMap.forEach((actionMapping, key) => {
        if (actionMapping.actionName === actionName)
          actionDescription.push(
            "  " + String.fromCharCode(key) + ": " + actionMapping.description
          );
      });
      actionDescription.push("");
    });

    this.controlsModal = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 5,
      [
        "Name: " + this.gdy.Environment.Name,
        "Description: " + this.gdy.Environment.Description,
        "",
        "Actions:",
        "",
        ...actionDescription,
      ]
    );
    this.controlsModal.setWordWrapWidth(this.cameras.main.width / 2);
    this.controlsModal.setBackgroundColor("#000000AA");
    this.controlsModal.setDepth(100);
    this.controlsModal.setOrigin(0.5, 0);
    this.controlsModal.setVisible(false);
  };

  init = (data) => {
    try {
      // Functions to interact with the environment
      this.jiddly = data.jiddly;

      // Data about the environment
      this.gdy = data.gdy;
      this.onDisplayMessage = data.onDisplayMessage;
      this.onTrajectoryComplete = data.onTrajectoryComplete;
      this.getTrajectory = data.getTrajectory;

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

    if (this.controlsModalActive) {
      this.controlsModal.setWordWrapWidth(this.cameras.main.width / 2);
      this.controlsModal.setFontFamily("Droid Sans Mono");
      this.controlsModal.setPosition(
        this.cameras.main.width / 2,
        this.cameras.main.height / 5
      );
    }
  }

  toggleVariableDebugModal() {
    this.variableDebugModalActive = !this.variableDebugModalActive;
    this.variableDebugModal.setVisible(this.variableDebugModalActive);
  }

  toggleControlsModal() {
    this.controlsModalActive = !this.controlsModalActive;
    this.controlsModal.setVisible(this.controlsModalActive);
  }

  setupKeyboardMapping = () => {
    const actionInputMappings = this.jiddly.getActionInputMappings();
    const actionNames = this.jiddly.getActionNames();

    const actionKeyOrder = [
      Phaser.Input.Keyboard.KeyCodes.THREE,
      Phaser.Input.Keyboard.KeyCodes.TWO,
      Phaser.Input.Keyboard.KeyCodes.ONE,
      Phaser.Input.Keyboard.KeyCodes.L,
      Phaser.Input.Keyboard.KeyCodes.O,
      Phaser.Input.Keyboard.KeyCodes.M,
      Phaser.Input.Keyboard.KeyCodes.K,
      Phaser.Input.Keyboard.KeyCodes.N,
      Phaser.Input.Keyboard.KeyCodes.J,
      Phaser.Input.Keyboard.KeyCodes.U,
      Phaser.Input.Keyboard.KeyCodes.B,
      Phaser.Input.Keyboard.KeyCodes.H,
      Phaser.Input.Keyboard.KeyCodes.Y,
      Phaser.Input.Keyboard.KeyCodes.V,
      Phaser.Input.Keyboard.KeyCodes.G,
      Phaser.Input.Keyboard.KeyCodes.T,
      Phaser.Input.Keyboard.KeyCodes.C,
      Phaser.Input.Keyboard.KeyCodes.F,
      Phaser.Input.Keyboard.KeyCodes.R,
      Phaser.Input.Keyboard.KeyCodes.Q,
      Phaser.Input.Keyboard.KeyCodes.E,
    ];

    const movementKeySets = [
      {
        "0,-1": Phaser.Input.Keyboard.KeyCodes.UP,
        "-1,0": Phaser.Input.Keyboard.KeyCodes.LEFT,
        "0,1": Phaser.Input.Keyboard.KeyCodes.DOWN,
        "1,0": Phaser.Input.Keyboard.KeyCodes.RIGHT,
      },
      {
        "0,-1": Phaser.Input.Keyboard.KeyCodes.W,
        "-1,0": Phaser.Input.Keyboard.KeyCodes.A,
        "0,1": Phaser.Input.Keyboard.KeyCodes.S,
        "1,0": Phaser.Input.Keyboard.KeyCodes.D,
      },
    ];

    this.input.keyboard.on("keydown-P", (event) => {
      this.toggleControlsModal();
    });

    this.input.keyboard.on("keydown-I", (event) => {
      this.toggleVariableDebugModal();
    });

    this.keyMap = new Map();

    actionNames.forEach((actionName, actionTypeId) => {
      const actionMapping = actionInputMappings[actionName];
      if (!actionMapping.internal) {
        const inputMappings = Object.entries(actionMapping.inputMappings);
        console.log(inputMappings);

        const actionDirections = new Set();
        inputMappings.forEach((inputMapping) => {
          // check that all the vectorToDest are different
          const mapping = inputMapping[1];
          actionDirections.add(this.toMovementKey(mapping.vectorToDest));
        });

        const directional = actionDirections.size !== 1;

        if (directional) {
          // pop movement keys
          const movementKeys = movementKeySets.pop();
          inputMappings.forEach((inputMapping) => {
            const actionId = Number(inputMapping[0]);
            const mapping = inputMapping[1];

            let key;
            if (this.toMovementKey(mapping.vectorToDest) in movementKeys) {
              key = movementKeys[this.toMovementKey(mapping.vectorToDest)];
            } else if (
              this.toMovementKey(mapping.orientationVector) in movementKeys
            ) {
              key = movementKeys[this.toMovementKey(mapping.orientationVector)];
            }
            this.keyMap.set(key, {
              actionName,
              actionTypeId,
              actionId,
              description: mapping.description,
            });
          });
        } else {
          // We have an action Key

          inputMappings.forEach((inputMapping) => {
            const key = actionKeyOrder.pop();

            const actionId = Number(inputMapping[0]);
            const mapping = inputMapping[1];

            this.keyMap.set(key, {
              actionName,
              actionTypeId,
              actionId,
              description: mapping.description,
            });
          });
        }
      }
    });

    const allKeys = {};

    this.keyMap.forEach((actionMapping, key) => {
      allKeys[key] = key;
    });

    this.keyboardMapping = this.input.keyboard.addKeys(allKeys, false);

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

  stopRecordingOrPlayback = () => {
    if(this.isRecordingTrajectory) {
      this.endRecording();
    }

    if(this.isRunningTrajectory) {
      this.endPlayback();
    }

    this.resetLevel();
  }

  beginRecording = () => {
    this.resetLevel();
    this.isRecordingTrajectory = true;
    this.currentTrajectoryBuffer = {
      steps: [],
    };
  };

  endRecording = () => {
    this.isRecordingTrajectory = false;

    const finalStep =
      this.currentTrajectoryBuffer.steps[
        this.currentTrajectoryBuffer.steps.length - 1
      ];

    // Check that the last step is terminated, otherwise just kill the buffer and shove a warning up
    if (finalStep.terminated) {
      this.onTrajectoryComplete(this.currentTrajectoryBuffer);
    } else {
      this.displayWarning(
        "Trajectory recording interrupted before finishing. This trajectory is now lost. :("
      );
    }
  };

  beginPlayback = () => {
    this.trajectoryActionIdx = 0;
    this.isRunningTrajectory = true;
  };

  endPlayback = () => {
    this.trajectoryActionIdx = 0;
    this.isRunningTrajectory = false;
  }

  resetLevel = () => {
    this.jiddly.reset();
  };

  processTrajectory = () => {
    if (this.currentTrajectoryBuffer.steps.length === 0) {
      this.isRunningTrajectory = false;
      return;
    }

    if (!this.cooldown) {
      this.cooldown = true;
      setTimeout(() => {
        this.cooldown = false;
      }, 50);

      const stateActionRewardTerminated =
        this.currentTrajectoryBuffer.steps[this.trajectoryActionIdx++];

      if (stateActionRewardTerminated.terminated) {
        this.isRunningTrajectory = false;
      }

      return stateActionRewardTerminated.state;
    }
  };

  processUserAction = () => {
    if (!this.cooldown) {
      this.cooldown = true;
      setTimeout(() => {
        this.cooldown = false;
      }, 100);

      let action = [];
      this.keyMap.forEach((actionMapping, key) => {
        if (this.keyboardMapping[key].isDown) {
          action.push(actionMapping.actionTypeId);
          action.push(actionMapping.actionId);
        }
      });

      if (action.length) {
        const stepResult = this.jiddly.step(action);

        this.globalVariableDebugText = this.getGlobalVariableDebugText();

        if (stepResult.reward > 0) {
          console.log("Reward: ", stepResult.reward);
        }

        const state = this.jiddly.getState();

        if (stepResult.terminated) {
          this.jiddly.reset();
        }

        if (this.isRecordingTrajectory) {
          const stateActionRewardTerminated = {
            state,
            action,
            reward: stepResult.reward,
            terminated: stepResult.terminated,
          };

          this.currentTrajectoryBuffer.steps.push(stateActionRewardTerminated);

          if (stepResult.terminated) {
            this.endRecording();
          }
        }
        return state;
      } else {
        return this.jiddly.getState();
      }
    }
  };

  preload = () => {
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
      this.mapping = this.setupKeyboardMapping();
      this.grenderer.init(this.gridWidth, this.gridHeight);
      this.updateState(this.jiddly.getState());
      this.createModals();
      this.updateModals();
      this.createTrajectoryMenu();
      this.updateTrajectoryMenu();
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
        if (this.isRunningTrajectory) {
          state = this.processTrajectory();
        } else if (this.isRecordingTrajectory) {
          state = this.processUserAction();
        } else {
          this.currentTrajectoryBuffer = this.getTrajectory();
          state = this.processUserAction();
        }

        if (state && this.stateHash !== state.hash) {
          this.stateHash = state.hash;
          this.updateState(state);
        }

        this.updateModals();
        this.updateTrajectoryMenu();
      }
    }
  };
}

export default HumanPlayerScene;
