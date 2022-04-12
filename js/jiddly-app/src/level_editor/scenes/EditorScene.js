import Phaser from "phaser";

class EditorScene extends Phaser.Scene {
  constructor() {
    super("EditorScene");
  }

  preload() {
    console.log("Editor Scene - Preload");
  }

  create() {
    console.log("Editor Scene - Create");

    this.text = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading Level Editor",
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

export default EditorScene;
