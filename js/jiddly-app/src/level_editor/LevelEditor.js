import { React, Component } from "react";
import Phaser from "phaser";

import EditorScene from "./scenes/EditorScene";
import { Col, Row } from "react-bootstrap";

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
      scene: [EditorScene],
    };

    this.game = new Phaser.Game(config);

    this.updateCanvasSize();
  }

  componentDidUpdate(prevProps) {
    this.updateCanvasSize();
    if (!prevProps.gdy && this.props.gdy) {
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
