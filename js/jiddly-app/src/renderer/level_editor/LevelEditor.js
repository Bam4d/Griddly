import { React, Component } from "react";
import Phaser from "phaser";

import EditorScene from "./scenes/EditorScene";
import { Col, Row } from "react-bootstrap";
import LoadingScene from "./../LoadingScene";

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
      this.game.scene.start("EditorScene", {
        gdy: this.props.gdy,
        editorStateHandler: this.props.editorStateHandler,
        rendererName: "Block2D"
      });
    } else if (prevProps.gdyHash !== this.props.gdyHash) {
      this.game.scene.getScene("EditorScene").scene.restart({
        gdy: this.props.gdy,
        editorStateHandler: this.props.editorStateHandler,
        rendererName: "Block2D"
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
