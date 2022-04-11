import { React, Component } from "react";
import Phaser from "phaser";

import RenderStateScene from "./scenes/RenderStateScene";
import LoadingScene from "./scenes/LoadingScene";

class JiddlyRenderer extends Component {
  constructor(props) {
    super(props);

    const config = {
      type: Phaser.AUTO,
      parent: "phaser-container",
      backgroundColor: "#282c34",
      scale: {
        mode: Phaser.Scale.ScaleModes.RESIZE,
        width: document.innerWidth,
        height: document.innerHeight,
      },
      scene: [LoadingScene, RenderStateScene],
    };

    this.game = new Phaser.Game(config);

    this.state = {
      loaded: false,
    };
  }

  getEnvState = () => {
    return this.props.envState;
  };

  envStep = (action) => {
    return this.props.envStep(action);
  };

  envReset = () => {
    return this.props.envReset();
  };

  componentDidUpdate(prevProps) {
    if (!prevProps.gdy && this.props.gdy) {
      this.game.scene.remove("LoadingScene");
      this.game.scene.start("RenderStateScene", {
        gdy: this.props.gdy,
        rendererName: "Sprite2D",
        getEnvState: this.getEnvState,
        envStep: this.envStep,
        envReset: this.envReset,
      });
    }
  }

  render() {
    return <></>;
  }
}

export default JiddlyRenderer;
