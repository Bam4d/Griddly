import Phaser from "phaser";

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
        font: "32px Arial",
        fill: "#ff0044",
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
