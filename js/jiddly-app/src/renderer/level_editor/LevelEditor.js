import { React, Component } from "react";
import Phaser from "phaser";

import EditorScene from "./scenes/EditorScene";
import { Col, Row } from "react-bootstrap";
import LoadingScene from "./scenes/LoadingScene";
import EditorState from "./EditorState";

class Player extends Component {
  updateCanvasSize = () => {
    this.game.scale.resize(this.props.width, this.props.height);
  };

  componentDidMount() {
    const config = {
      type: Phaser.AUTO,
      parent: this.divElement,
      backgroundColor: "#000000",
      scale: {
        // mode: Phaser.Scale.ScaleModes.RESIZE,
        expandParent: false,
      },
      scene: [LoadingScene, EditorScene],
    };

    

    this.game = new Phaser.Game(config);

    this.updateCanvasSize();
  }

  componentDidUpdate(prevProps) {
    this.updateCanvasSize();

    if (prevProps.gdyHash === 0 && this.props.gdy) {
      this.game.scene.remove("LoadingScene");

      const editorState = new EditorState(this.props.gdy);

      const levels = this.props.gdy.Environment.Levels;

      this.editorState.loadLevelString(levels[this.props.levelId]);

      this.game.scene.start("EditorScene", {
        editorState,
      });
    } else if (prevProps.gdyHash !== this.props.gdyHash) {

      const editorState = new EditorState(this.props.gdy);

      this.game.scene.getScene("EditorScene").scene.restart({
        editorState,
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

export default Player;
