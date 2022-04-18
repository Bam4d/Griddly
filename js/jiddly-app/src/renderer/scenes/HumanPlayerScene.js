import Phaser from "phaser";
import Block2DRenderer from "./Block2DRenderer";
import Sprite2DRenderer from "./Sprite2DRenderer";

class HumanPlayerScene extends Phaser.Scene {
  constructor() {
    super("HumanPlayerScene");

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
          playerVariableDescription.push(variableName + ":");
          for (let p = 0; p < this.jiddly.playerCount; p++) {
            const variableValue = variableData[p + 1];
            playerVariableDescription.push(
              "  " + (p + 1) + ": " + variableValue
            );
          }
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
      this.variableDebugModal.setWordWrapWidth(this.cameras.main.width / 2);
      this.variableDebugModal.setPosition(0, 0);
      this.variableDebugModal.setText(this.getGlobalVariableDebugText());
    }

    if (this.controlsModalActive) {
      this.controlsModal.setWordWrapWidth(this.cameras.main.width / 2);
      this.controlsModal.setPosition(
        this.cameras.main.width / 2,
        this.cameras.main.height / 5
      );
    }
  }

  toggleVariableDebugModal() {
    if (!this.variableDebugModalActive) {
      if (!this.variableDebugModal) {
        // Get all the global variables
        this.variableDebugModal = this.add.text(
          this.cameras.main.width / 2,
          this.cameras.main.height / 5,
          this.getGlobalVariableDebugText()
        );
        this.variableDebugModal.setBackgroundColor("#000000AA");
        this.variableDebugModal.setDepth(100);
        this.variableDebugModal.setOrigin(0, 0);
      }
      this.variableDebugModalActive = true;
    } else {
      this.variableDebugModalActive = false;
    }

    this.variableDebugModal.setVisible(this.variableDebugModalActive);
  }

  toggleControlsModal() {
    if (!this.controlsModalActive) {
      if (!this.controlsModal) {
        const actionDescription = [];

        const actionNames = this.jiddly.getActionNames();
        actionNames.forEach((actionName) => {
          actionDescription.push(actionName + ": ");
          this.keyMap.forEach((actionMapping, key) => {
            if (actionMapping.actionName === actionName)
              actionDescription.push(
                "  " +
                  String.fromCharCode(key) +
                  ": " +
                  actionMapping.description
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
      }
      this.controlsModalActive = true;
    } else {
      this.controlsModalActive = false;
    }

    this.controlsModal.setVisible(this.controlsModalActive);
  }

  setupKeyboardMapping = () => {
    const actionInputMappings = this.jiddly.getActionInputMappings();
    const actionNames = this.jiddly.getActionNames();

    const actionKeyOrder = [
      Phaser.Input.Keyboard.KeyCodes.O,
      Phaser.Input.Keyboard.KeyCodes.U,
      Phaser.Input.Keyboard.KeyCodes.M,
      Phaser.Input.Keyboard.KeyCodes.N,
      Phaser.Input.Keyboard.KeyCodes.B,
      Phaser.Input.Keyboard.KeyCodes.H,
      Phaser.Input.Keyboard.KeyCodes.Y,
      Phaser.Input.Keyboard.KeyCodes.G,
      Phaser.Input.Keyboard.KeyCodes.V,
      Phaser.Input.Keyboard.KeyCodes.Z,
      Phaser.Input.Keyboard.KeyCodes.X,
      Phaser.Input.Keyboard.KeyCodes.C,
      Phaser.Input.Keyboard.KeyCodes.F,
      Phaser.Input.Keyboard.KeyCodes.R,
      Phaser.Input.Keyboard.KeyCodes.Q,
      Phaser.Input.Keyboard.KeyCodes.E,
    ];

    const movementKeySets = [
      {
        "0,-1": Phaser.Input.Keyboard.KeyCodes.I,
        "-1,0": Phaser.Input.Keyboard.KeyCodes.J,
        "0,1": Phaser.Input.Keyboard.KeyCodes.K,
        "1,0": Phaser.Input.Keyboard.KeyCodes.L,
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
        console.log("Action: ", action);
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

      this.updateModals();
    }
  };
}

export default HumanPlayerScene;
