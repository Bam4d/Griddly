import { React, Component } from "react";
import EditorStateHandler from "../../LevelEditorStateHandler";
import Phaser from "phaser";

import { Col, Row } from "react-bootstrap";
import LoadingScene from "../LoadingScene";
import PreviewScene from "./scenes/LevelCarouselScene";
import { hashString } from "../../Utils";

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

  updateStateHandlers() {
    this.props.gdy.Environment.Levels.forEach((levelString, idx) => {
      const stateHandler = this.stateHandlers.get(idx);
      if (stateHandler) {
        const state = stateHandler.getState();
        if (state.hash !== hashString(levelString)) {
          const updatedStateHandler = new EditorStateHandler();
          updatedStateHandler.loadGDY(this.props.gdy);
          updatedStateHandler.loadLevelString(levelString);
          this.stateHandlers.set(idx, updatedStateHandler);
        }
      } else {
        const newStateHandler = new EditorStateHandler();
        newStateHandler.loadGDY(this.props.gdy);
        newStateHandler.loadLevelString(levelString);
        this.stateHandlers.set(idx, newStateHandler);
      }
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
        selectedLevelId: 0
      });
    } else if (prevProps.gdyHash !== this.props.gdyHash) {
      this.updateStateHandlers();

      this.game.scene.getScene("PreviewScene").scene.restart({
        gdy: this.props.gdy,
        editorStateHandlers: this.stateHandlers,
        rendererConfig: this.props.rendererConfig,
        rendererName: this.props.rendererName,
        onSelectLevel: this.props.onSelectLevel,
        selectedLevelId: this.props.selectedLevelId
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
