
class RendererBase {
  constructor(scene, rendererName, renderConfig, avatarObject) {
    this.scene = scene;
    this.rendererName = rendererName;
    this.renderConfig = renderConfig;
    this.avatarObject = avatarObject;

    if (!this.scene.imagePaths) {
      this.scene.imagePaths = {};
    }
  }

  init(gridWidth, gridHeight, container) {
    this.gridHeight = gridHeight;
    this.gridWidth = gridWidth;
    this.container = container;
  };

  beginUpdate(objects) {
    // We dont really have to do anything here
  };

  getCenteredX = (x) => {
    return (
      this.scene.cameras.main.centerX +
      (x - this.gridWidth / 2.0 + 0.5) * this.renderConfig.TileSize
    );
  };

  getCenteredY = (y) => {
    return (
      this.scene.cameras.main.centerY +
      (y - this.gridHeight / 2.0 + 0.5) * this.renderConfig.TileSize
    );
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
