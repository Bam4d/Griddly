class EditorState {
  constructor() {
    this.defaultTileSize = 24;
    this.objectTemplates = {};

    this.characterToObject = {};
  }

  loadTemplates(gdy, rendererName, rendererConfig) {

    if ("BackgroundTile" in rendererConfig[rendererName]) {

      const objectTemplate = {
        name: "background",
        id: "background",
        tilingMode: "NONE",
        scale: 1.0,
        color: { r: 1, g: 1, b: 1 },
        zIdx: 0,
        images: [rendererConfig[rendererName]["BackgroundTile"]]
      };

      this.objectTemplates[rendererName]["background"] = objectTemplate;
      this.characterToObject["."] = "background";

    }

    gdy.Objects.forEach((object) => {
      const rendererConfig = object.Observers[rendererName];

      for (let idx = 0; idx < rendererConfig.length; idx++) {
        const config = rendererConfig[idx];

        let images = [];
        if (Array.isArray(config.Image)) {
          images = config.Image;
        } else {
          images.push(config.Image);
        }

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
          images,
        };

        this.objectTemplates[rendererName][objectTemplate.id] = objectTemplate;
        this.characterToObject[object.MapCharacter] = objectTemplate.id;
      }
    });
  }

  loadRenderers(gdy) {
    const observers = gdy.Environment.Observers;

    this.rendererConfigs = [];

    for (const rendererName in observers) {
      const rendererConfig = observers[rendererName];

      if (!("TileSize" in rendererConfig)) {
        rendererConfig["TileSize"] = this.defaultTileSize;
      }

      if (!("Type" in rendererConfig)) {
        if (rendererName === "SPRITE_2D" || rendererName === "Sprite2D") {
          rendererConfig["Type"] = "SPRITE_2D";
        } else if (rendererName === "BLOCK_2D" || rendererName === "Block2D") {
          rendererConfig["Type"] = "BLOCK_2D";
        } else {
          this.displayError(
            "Only Block2D and Sprite2D renderers can be used to view Jiddly environments"
          );
        }

        this.loadTemplates(gdy, rendererName, rendererConfig);

        this.rendererConfigs[rendererName] = rendererConfig;
      }
    }
  }

  loadGDY(gdy) {
    this.loadRenderers(gdy);
  }

  loadLevelString(levelString) {
    

  }

  addTile() {}

  removeTile() {}

  toLevelString() {}
}

export default EditorState;
