import Phaser from "phaser";
import { COLOR_LOADING_TEXT } from "./ThemeConsts";

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
        fontSize: "32px",
        fill: COLOR_LOADING_TEXT,
        align: "center",
      }
    );

    this.running = this.add.text(0, 0, "\uf70c", {
      fontFamily: "Font Awesome Solid",
      fontSize: "32px",
      fill: COLOR_LOADING_TEXT,
      align: "center",
    });

    this.runningSolid = this.add.text(32, 0, "\uf70c", {
      fontFamily: "Font Awesome Solid",
      fontSize: "32px",
      fill: COLOR_LOADING_TEXT,
      align: "center",
    });
  }

  update() {
    this.text.setX(this.cameras.main.width / 2);
    this.text.setY(this.cameras.main.height / 2);
    this.text.setOrigin(0.5, 0.5);
  }
}

export default LoadingScene;
