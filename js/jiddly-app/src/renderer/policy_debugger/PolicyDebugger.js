import { React, Component } from "react";
import Phaser from "phaser";

import PolicyDebuggerScene from "./scenes/PolicyDebuggerScene";
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
      scene: [LoadingScene, PolicyDebuggerScene],
    };

    this.game = new Phaser.Game(config);

    this.updateCanvasSize();
  }

  getTrajectory = () => {
    if (this.props.selectedLevelId in this.props.trajectories && this.props.trajectories[this.props.selectedLevelId]) {
      return this.props.trajectories[this.props.selectedLevelId][0];
    } else {
      return;
    }
  };

  componentDidUpdate(prevProps) {
    this.updateCanvasSize();

    if (this.props.jiddly) {
      if (prevProps.gdyHash === 0 && this.props.gdy) {
        this.game.scene.remove("LoadingScene");
        this.game.scene.start("PolicyDebuggerScene", {
          gdy: this.props.gdy,
          rendererConfig: this.props.rendererConfig,
          rendererName: this.props.rendererName,
          jiddly: this.props.jiddly,
          onDisplayMessage: this.props.onDisplayMessage,
          onTrajectoryComplete: this.props.onTrajectoryComplete,
        });
      } else if (prevProps.gdyHash !== this.props.gdyHash) {
        this.game.scene.getScene("PolicyDebuggerScene").scene.restart({
          gdy: this.props.gdy,
          rendererConfig: this.props.rendererConfig,
          rendererName: this.props.rendererName,
          jiddly: this.props.jiddly,
          onDisplayMessage: this.props.onDisplayMessage,
          onTrajectoryComplete: this.props.onTrajectoryComplete,
        });
      }
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
