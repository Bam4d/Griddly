import { React, Component } from "react";
import Phaser from "phaser";

import PolicyDebuggerScene from "./scenes/PolicyDebuggerScene";
import LoadingScene from "../LoadingScene";

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
      scene: [LoadingScene, PolicyDebuggerScene],
    };

    this.game = new Phaser.Game(config);

    this.gdyHash = 0;

    this.updateCanvasSize();
  }

  componentDidUpdate(prevProps) {
    this.updateCanvasSize();

    if (this.props.griddlyjs) {
      if (this.gdyHash === 0 && this.props.gdy) {
        this.game.scene.remove("LoadingScene");
        this.game.scene.start("PolicyDebuggerScene", {
          gdy: this.props.gdy,
          rendererConfig: this.props.rendererConfig,
          rendererName: this.props.rendererName,
          griddlyjs: this.props.griddlyjs,
          onDisplayMessage: this.props.onDisplayMessage,
          onTrajectoryComplete: this.props.onTrajectoryComplete,
          model: this.props.model,
        });
      } else if (this.gdyHash !== this.props.gdyHash) {
        this.game.scene.getScene("PolicyDebuggerScene").scene.restart({
          gdy: this.props.gdy,
          rendererConfig: this.props.rendererConfig,
          rendererName: this.props.rendererName,
          griddlyjs: this.props.griddlyjs,
          onDisplayMessage: this.props.onDisplayMessage,
          onTrajectoryComplete: this.props.onTrajectoryComplete,
          model: this.props.model,
        });
      }
    }
    this.gdyHash = this.props.gdyHash;
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
