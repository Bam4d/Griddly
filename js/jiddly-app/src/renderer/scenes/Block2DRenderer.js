import Phaser from "phaser";

class Block2DRenderer {

  constructor(scene, tileSize=32) {
    this.scene = scene;
    this.tileSize = tileSize

    this.objectTemplates = {};
  }

  addObject = (objectTemplateName, x, y, orientation) => {
    const objectTemplate = this.objectTemplates[objectTemplateName];
    const sprite = this.scene.add.sprite(
      x * this.tileSize,
      y * this.tileSize,
      objectTemplate.id
    );

    sprite.setDisplaySize(this.tileSize, this.tileSize);
    //sprite.setOrigin(0, 0);
    sprite.setTint(
      Phaser.Display.Color.GetColor(
        objectTemplate.color.r * 255,
        objectTemplate.color.g * 255,
        objectTemplate.color.b * 255
      )
    );

    sprite.setRotation(this.getOrientationAngleRads(orientation));
    sprite.setScale(objectTemplate.scale);
    sprite.setDepth(objectTemplate.zIdx);

    return sprite;
  };

  updateObject = (sprite, objectTemplateName, x, y, orientation) => {
    const objectTemplate = this.objectTemplates[objectTemplateName];
    sprite.setPosition(x * this.tileSize, y * this.tileSize);
    sprite.setTexture(objectTemplate.id);

    sprite.setTint(
      Phaser.Display.Color.GetColor(
        objectTemplate.color.r * 255,
        objectTemplate.color.g * 255,
        objectTemplate.color.b * 255
      )
    );

    sprite.setRotation(this.getOrientationAngleRads(orientation));
    sprite.setScale(objectTemplate.scale);
    sprite.setDepth(objectTemplate.zIdx);
  };

  loadTemplates = (objects) => {
    this.scene.load.baseURL = "resources/images/";
    objects.forEach((object) => {
      const block2DConfig = object.Observers.Block2D;

      for (let idx = 0; idx < block2DConfig.length; idx++) {
        const config = block2DConfig[idx];

        const objectTemplate = {
          id: object.Name + idx,
          scale: config.Scale,
          color: {
            r: config.Color[0],
            g: config.Color[1],
            b: config.Color[2],
          },
          zIdx: object.Z || 0,
        };

        this.scene.load.image(
          objectTemplate.id,
          this.getShapeImage(config.Shape)
        );

        this.objectTemplates[objectTemplate.id] = objectTemplate;
      }
    });
  };

  getShapeImage = (shape) => {
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

  getOrientationAngleRads = (orientation) => {
    switch (orientation) {
      default:
      case "NONE":
      case "UP":
        return 0;
      case "RIGHT":
        return Math.PI / 2.0;
      case "DOWN":
        return Math.PI;
      case "LEFT":
        return (3.0 * Math.PI) / 2.0;
    }
  };
}

export default Block2DRenderer;
