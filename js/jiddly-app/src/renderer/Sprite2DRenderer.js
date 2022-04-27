import Phaser from "phaser";
import RendererBase from "./RendererBase";

class Sprite2DRenderer extends RendererBase {
  constructor(scene, rendererName, renderConfig, avatarObject) {
    super(scene, rendererName, renderConfig, avatarObject);

    this.objectTemplates = {};

    this.tileLocations = new Map();
  }

  init(gridWidth, gridHeight) {
    super.init(gridWidth, gridHeight);

    if ("BackgroundTile" in this.renderConfig) {
      const sprite = this.scene.add.tileSprite(
        this.scene.cameras.main.centerX,
        this.scene.cameras.main.centerY,
        gridWidth * this.renderConfig.TileSize,
        gridHeight * this.renderConfig.TileSize,
        "__background__"
      );

      const backgroundSourceImage =
        this.scene.textures.get("__background__").source[0];

      sprite.tileScaleX =
        this.renderConfig.TileSize / backgroundSourceImage.width;
      sprite.tileScaleY =
        this.renderConfig.TileSize / backgroundSourceImage.height;

      this.backgroundSprite = sprite;
    }
  }

  beginUpdate(objects) {
    super.beginUpdate(objects);
    if (this.backgroundSprite) {
      this.backgroundSprite.setPosition(
        this.scene.cameras.main.centerX,
        this.scene.cameras.main.centerY
      );
    }

    this.tileLocations.clear();
    objects.forEach((object) => {
      this.tileLocations.set(
        this.getObjectLocationKey(object.location.x, object.location.y),
        object.name
      );
    });
  }

  getObjectLocationKey = (x, y) => {
    return `${x},${y}`;
  };

  addObject = (objectName, objectTemplateName, x, y, orientation) => {
    const objectTemplate = this.objectTemplates[objectTemplateName];

    const sprite = this.scene.add.sprite(
      this.getCenteredX(x),
      this.getCenteredY(y),
      this.getTilingImage(objectTemplate, x, y)
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

    if (this.avatarObject !== objectName) {
      sprite.setRotation(this.getOrientationAngleRads(orientation));
    } else if (this.renderConfig.RotateAvatarImage) {
      sprite.setRotation(this.getOrientationAngleRads(orientation));
    }
    sprite.setDepth(objectTemplate.zIdx);

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
    sprite.setTexture(this.getTilingImage(objectTemplate, x, y));

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

    if ("BackgroundTile" in this.renderConfig) {
      this.loadImage("__background__", this.renderConfig.BackgroundTile);
    }

    objects.forEach((object) => {
      const sprite2DConfig = object.Observers[this.rendererName];

      for (let idx = 0; idx < sprite2DConfig.length; idx++) {
        const config = sprite2DConfig[idx];

        if (Array.isArray(config.Image)) {
          const objectTemplate = {
            name: object.Name,
            id: object.Name + idx,
            tilingMode: config.TilingMode || "NONE",
            scale: config.Scale || 1.0,
            color: config.Color
              ? {
                  r: config.Color[0],
                  g: config.Color[1],
                  b: config.Color[2],
                }
              : { r: 1, g: 1, b: 1 },
            zIdx: object.Z || 0,
          };

          for (let t = 0; t < config.Image.length; t++) {
            this.loadImage(objectTemplate.id + t, config.Image[t]);
          }

          this.objectTemplates[objectTemplate.id] = objectTemplate;
        } else {
          const objectTemplate = {
            name: object.Name,
            id: object.Name + idx,
            tilingMode: "NONE",
            scale: config.Scale || 1.0,
            color: config.Color
              ? {
                  r: config.Color[0],
                  g: config.Color[1],
                  b: config.Color[2],
                }
              : { r: 1, g: 1, b: 1 },
            zIdx: object.Z || 0,
          };

          this.loadImage(objectTemplate.id, config.Image);

          this.objectTemplates[objectTemplate.id] = objectTemplate;
        }
      }
    });
  };

  getTilingImage = (objectTemplate, x, y) => {
    if (objectTemplate.tilingMode === "WALL_16") {
      const objectLeft = this.tileLocations.get(
        this.getObjectLocationKey(x - 1, y)
      );
      const objectRight = this.tileLocations.get(
        this.getObjectLocationKey(x + 1, y)
      );
      const objectUp = this.tileLocations.get(
        this.getObjectLocationKey(x, y - 1)
      );
      const objectDown = this.tileLocations.get(
        this.getObjectLocationKey(x, y + 1)
      );

      let idx = 0;
      if (objectLeft && objectLeft === objectTemplate.name) {
        idx += 1;
      }
      if (objectRight && objectRight === objectTemplate.name) {
        idx += 2;
      }
      if (objectUp && objectUp === objectTemplate.name) {
        idx += 4;
      }
      if (objectDown && objectDown === objectTemplate.name) {
        idx += 8;
      }

      return objectTemplate.id + idx;
    } else if (objectTemplate.tilingMode === "WALL_2") {
      const objectDown = this.tileLocations.get(
        this.getObjectLocationKey(x, y - 1)
      );
      let idx = 0;
      if (objectDown && objectDown === objectTemplate.name) {
        idx += 1;
      }

      return objectTemplate.id + idx;
    }

    return objectTemplate.id;
  };
}

export default Sprite2DRenderer;
