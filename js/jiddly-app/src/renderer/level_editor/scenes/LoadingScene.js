import Phaser from "phaser";

const COLOR_LOADING = "#3dc9b0";

class LoadingScene extends Phaser.Scene {
  constructor() {
    super("LoadingScene");
  }

  preload() {
    console.log("Loading Scene - Preload");
  }

  create() {
    console.log("Loading Scene - Create");

    this.text = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading Griddly GDY",
      {
        fontFamily: "Droid Sans Mono",
        font: "32px",
        fill: COLOR_LOADING,
        align: "center",
      }
    );
  }

  update() {
    this.text.setX(this.cameras.main.width / 2);
    this.text.setY(this.cameras.main.height / 2);
    this.text.setOrigin(0.5, 0.5);
  }
}

export default LoadingScene;
