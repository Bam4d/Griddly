import Phaser from "phaser";

class Sprite2DRenderer {
  constructor(scene, tileSize = 32) {
    this.scene = scene;
    this.tileSize = tileSize;

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
      const sprite2DConfig = object.Observers.Sprite2D;

      for (let idx = 0; idx < sprite2DConfig.length; idx++) {
        const config = sprite2DConfig[idx];

        if (Array.isArray(config.image)) {
          const objectTemplate = {
            id: object.Name + idx,
            scale: config.Scale,
            color: config.Color
              ? {
                  r: config.Color[0],
                  g: config.Color[1],
                  b: config.Color[2],
                }
              : { r: 1, g: 1, b: 1 },
            zIdx: object.Z || 0,
          };

          this.scene.load.image(
            objectTemplate.id,
            this.getSpriteImage(config.Image[0])
          );

          this.objectTemplates[objectTemplate.id] = objectTemplate;
        } else {
          const objectTemplate = {
            id: object.Name + idx,
            scale: config.Scale,
            color: config.Color
              ? {
                  r: config.Color[0],
                  g: config.Color[1],
                  b: config.Color[2],
                }
              : { r: 1, g: 1, b: 1 },
            zIdx: object.Z || 0,
          };

          this.scene.load.image(
            objectTemplate.id,
            this.getSpriteImage(config.Image)
          );

          this.objectTemplates[objectTemplate.id] = objectTemplate;
        }
      }
    });
  };

  getSpriteImage = (image) => {
    return image;
  }

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

export default Sprite2DRenderer;
