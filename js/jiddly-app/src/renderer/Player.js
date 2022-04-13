import { React, Component } from "react";
import Phaser from "phaser";

import RenderStateScene from "./scenes/RenderStateScene";
import LoadingScene from "./scenes/LoadingScene";

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
      scene: [LoadingScene, RenderStateScene],
    };

    this.game = new Phaser.Game(config);

    this.updateCanvasSize();
  }

  componentDidUpdate(prevProps) {
    this.updateCanvasSize();

    if (prevProps.gdyHash === 0 && this.props.gdy) {
      this.game.scene.remove("LoadingScene");
      this.game.scene.start("RenderStateScene", {
        gdy: this.props.gdy,
        rendererName: "Sprite2D",
        jiddly: this.props.jiddly,
      });
    } else if (prevProps.gdyHash !== this.props.gdyHash) {
      this.game.scene.getScene("RenderStateScene").scene.restart({
        gdy: this.props.gdy,
        rendererName: "Sprite2D",
        jiddly: this.props.jiddly,
      });
    }
  }

  render() {
    return (
      <div
        ref={(divElement) => {
          this.divElement = divElement;
        }}
      ></div>
    );
  }
}

export default Player;
