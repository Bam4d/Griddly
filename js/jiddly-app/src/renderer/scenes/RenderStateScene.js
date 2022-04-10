import Phaser from "phaser";

class RenderStateScene extends Phaser.Scene {
  constructor() {
    super("RenderStateScene");
  }

  init = (data) => {
    this.data = data;
  };

  displayError = (error) => {};

  getBlock2DShapeImage = (shape) => {
    switch (shape) {
      case "circle":
        return "block_shapes/circle.png";
      case "triangle":
        return "block_shapes/triangle.png";
      case "square":
        return "block_shapes/square.png";
      case "pentagon":
        return "block_shapes/pentagon.png";
      case "hexagon":
        return "block_shapes/hexagon.png";
      default:
        this.displayError("Cannot find image for BLOCK_2D shape " + shape);
        return "block_shapes/square.png";
    }
  };

  addBlock2DSprite = (objectTemplate, x, y, rotation) => {
    const sprite = this.add.sprite(x, y, objectTemplate.id);
    sprite.setTint(
      Phaser.Display.Color.GetColor(
        objectTemplate.color.r,
        objectTemplate.color.g,
        objectTemplate.color.b
      )
    );
    return;
  };

  updateBlock2DSprite = (sprite, objectTemplate, x, y, rotation) => {};

  removeBlock2DSprite = (sprite) => {};

  loadBlock2DTemplates = (objects) => {
    const objectTemplates = {};
    this.load.baseURL = "resources/images/";
    objects.map((object) => {
      const block2DConfig = object.Observers.Block2D;

      for (let idx = 0; idx < block2DConfig.length; idx++) {
        const config = block2DConfig[idx];

        const objectTemplate = {
          id: object.Name + idx,
          shape: config.Shape,
          scale: config.Scale,
          color: {
            r: config.Color[0],
            g: config.Color[1],
            b: config.Color[2],
          },
        };

        this.load.image(
          objectTemplate.id,
          this.getBlock2DShapeImage(objectTemplate.shape)
        );

        objectTemplates[objectTemplate.id] = objectTemplate;
      }
    });

    return objectTemplates;
  };

  preload = () => {
    console.log("Preload", this.data);

    const envName = this.data.gdy.Environment.Name;
    //const envDescription = this.data.gdy.Environment.Description;

    this.text = this.add.text(
      this.cameras.main.width / 2,
      this.cameras.main.height / 2,
      "Loading assets for " + envName,
      {
        font: "65px Arial",
        fill: "#ff0044",
        align: "center",
      }
    );

    this.loadBlock2DTemplates(this.data.gdy.Objects);
  };

  create = () => {
    console.log("Create");
  };

  update = () => {
    this.text.setX(this.cameras.main.width / 2);
    this.text.setY(this.cameras.main.height / 2);
    this.text.setOrigin(0.5, 0.5);
  };
}

export default RenderStateScene;
