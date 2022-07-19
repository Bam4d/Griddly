class RendererBase {
  constructor(
    scene,
    rendererName,
    renderConfig,
    avatarObject,
    centerObjects = true
  ) {
    this.scene = scene;
    this.rendererName = rendererName;
    this.renderConfig = renderConfig;
    this.avatarObject = avatarObject;
    this.centerObjects = centerObjects;

    if (!this.scene.imagePaths) {
      this.scene.imagePaths = {};
    }
  }

  init(gridWidth, gridHeight, container) {
    this.gridWidth = gridWidth;
    this.gridHeight = gridHeight;
    this.container = container;
  }

  beginUpdate(objects, state) {}

  recenter(gridWidth, gridHeight) {
    this.gridWidth = gridWidth;
    this.gridHeight = gridHeight;
  }

  getCenteredX = (x) => {
    const scaledX =
      (x - this.gridWidth / 2.0 + 0.5) * this.renderConfig.TileSize;
    if (this.centerObjects) {
      return scaledX + this.scene.cameras.main.centerX;
    } else {
      return scaledX;
    }
  };

  getCenteredY = (y) => {
    const scaledY =
      (y - this.gridHeight / 2.0 + 0.5) * this.renderConfig.TileSize;
    if (this.centerObjects) {
      return scaledY + this.scene.cameras.main.centerY;
    } else {
      return scaledY;
    }
  };

  loadImage = (image, imagePath) => {
    if (
      image in this.scene.imagePaths &&
      this.scene.imagePaths[image] !== imagePath
    ) {
      this.scene.textures.remove(image);
    }

    this.scene.imagePaths[image] = imagePath;

    this.scene.load.image(image, imagePath);
  };

  getTilingImage = (objectTemplate, x, y) => {
    return objectTemplate.id;
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

export default RendererBase;
