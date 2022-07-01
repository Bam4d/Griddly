import { React, Component } from "react";
import EditorStateHandler from "../../LevelEditorStateHandler";
import Phaser from "phaser";

import { Col, Row } from "react-bootstrap";
import LoadingScene from "../LoadingScene";
import PreviewScene from "./scenes/LevelCarouselScene";

class LevelSelector extends Component {
  updateCanvasSize = () => {
    this.game.scale.resize(this.props.width, this.props.height);
  };

  componentDidMount() {
    const config = {
      type: Phaser.AUTO,
      parent: this.divElement,
      backgroundColor: "#000000",
      scale: {
        expandParent: false,
      },
      scene: [LoadingScene, PreviewScene],
    };

    this.game = new Phaser.Game(config);

    this.stateHandlers = new Map();

    this.updateCanvasSize();
  }

  displayError = (message, error) => {
    this.props.onDisplayMessage(message, "error", error);
  };

  displayWarning = (message, error) => {
    this.props.onDisplayMessage(message, "warning", error);
  };

  updateStateHandlers() {
    this.stateHandlers.clear();
    this.props.gdy.Environment.Levels.forEach((levelString, idx) => {
      const newStateHandler = new EditorStateHandler();
      try {
        newStateHandler.loadGDY(this.props.gdy);
        newStateHandler.loadLevelString(levelString);
      } catch (error) {
        this.displayError("Enable to load level" + idx);
      }
      this.stateHandlers.set(idx, newStateHandler);
    });
  }

  componentDidUpdate(prevProps) {
    this.updateCanvasSize();

    if (prevProps.gdyHash === 0 && this.props.gdy) {
      this.updateStateHandlers();

      this.game.scene.remove("LoadingScene");
      this.game.scene.start("PreviewScene", {
        gdy: this.props.gdy,
        editorStateHandlers: this.stateHandlers,
        rendererConfig: this.props.rendererConfig,
        rendererName: this.props.rendererName,
        onSelectLevel: this.props.onSelectLevel,
        getSelectedLevelId: () => this.props.selectedLevelId,
      });
    } else if (prevProps.gdyHash !== this.props.gdyHash) {
      this.updateStateHandlers();

      this.game.scene.getScene("PreviewScene").scene.restart({
        gdy: this.props.gdy,
        editorStateHandlers: this.stateHandlers,
        rendererConfig: this.props.rendererConfig,
        rendererName: this.props.rendererName,
        onSelectLevel: this.props.onSelectLevel,
        getSelectedLevelId: () => this.props.selectedLevelId,
      });
    }
  }

  render() {
    return (
      <>
        <Row>
          <Col>
            <div
              ref={(divElement) => {
                this.divElement = divElement;
              }}
            ></div>
          </Col>
        </Row>
        <Row>
          <Col></Col>
        </Row>
      </>
    );
  }
}

export default LevelSelector;
