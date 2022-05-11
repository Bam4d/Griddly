import Phaser from "phaser";
import RendererBase from "./RendererBase";

class Block2DRenderer extends RendererBase {
  constructor(scene, rendererName, renderConfig, avatarObject) {
    super(scene, rendererName, renderConfig, avatarObject);
    this.objectTemplates = {};
  }

  addObject = (objectName, objectTemplateName, x, y, orientation) => {
    const objectTemplate = this.objectTemplates[objectTemplateName];
    const sprite = this.scene.add.sprite(
      this.getCenteredX(x),
      this.getCenteredY(y),
      objectTemplate.id
    );

    sprite.setDisplaySize(
      this.renderConfig.TileSize * objectTemplate.scale,
      this.renderConfig.TileSize * objectTemplate.scale
    );
    //sprite.setOrigin(0, 0);
    sprite.setTint(
      Phaser.Display.Color.GetColor(
        objectTemplate.color.r * 255,
        objectTemplate.color.g * 255,
        objectTemplate.color.b * 255
      )
    );

    sprite.setRotation(this.getOrientationAngleRads(orientation));
    sprite.setDepth(objectTemplate.zIdx);
    
    if(this.container) {
      this.container.add(sprite);
    }

    return sprite;
  };

  updateObject = (
    sprite,
    objectName,
    objectTemplateName,
    x,
    y,
    orientation
  ) => {
    const objectTemplate = this.objectTemplates[objectTemplateName];
    sprite.setPosition(this.getCenteredX(x), this.getCenteredY(y));
    sprite.setTexture(objectTemplate.id);

    sprite.setDisplaySize(
      this.renderConfig.TileSize * objectTemplate.scale,
      this.renderConfig.TileSize * objectTemplate.scale
    );

    sprite.setTint(
      Phaser.Display.Color.GetColor(
        objectTemplate.color.r * 255,
        objectTemplate.color.g * 255,
        objectTemplate.color.b * 255
      )
    );

    if (this.avatarObject !== objectName) {
      sprite.setRotation(this.getOrientationAngleRads(orientation));
    } else if (this.renderConfig.RotateAvatarImage) {
      sprite.setRotation(this.getOrientationAngleRads(orientation));
    }

    sprite.setDepth(objectTemplate.zIdx);
  };

  loadTemplates = (objects) => {
    this.scene.load.baseURL = "resources/images/";
    objects.forEach((object) => {
      const block2DConfig = object.Observers[this.rendererName];

      for (let idx = 0; idx < block2DConfig.length; idx++) {
        const config = block2DConfig[idx];

        const color = config.Color || [1,1,1];

        const objectTemplate = {
          name: object.Name,
          id: object.Name + idx,
          scale: config.Scale || 1.0,
          color: {
            r: color[0],
            g: color[1],
            b: color[2],
          },
          zIdx: object.Z || 0,
        };

        this.loadImage(objectTemplate.id, this.getShapeImage(config.Shape));

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
}

export default Block2DRenderer;
