import Sprite2DRenderer from "../Sprite2DRenderer";

class EditorState {
  constructor(gdy) {
    this.defaultTileSize = 24;
    this.objectTemplates = {};

    this.characterToObject = {};

    this.gdy = gdy;

    this.loadRenderers(gdy);
  }

  // loadTemplates(gdy, rendererName, rendererConfig) {

  //   this.objectTemplates[rendererName] = {};

  //   if (rendererName in rendererConfig && "BackgroundTile" in rendererConfig[rendererName]) {

  //     const objectTemplate = {
  //       name: "background",
  //       id: "background",
  //       tilingMode: "NONE",
  //       scale: 1.0,
  //       color: { r: 1, g: 1, b: 1 },
  //       zIdx: 0,
  //       images: [rendererConfig[rendererName]["BackgroundTile"]]
  //     };

  //     this.objectTemplates[rendererName]["background"] = objectTemplate;
  //     this.characterToObject["."] = "background";

  //   }

  //   gdy.Objects.forEach((object) => {
  //     const rendererConfig = object.Observers[rendererName];

  //     for (let idx = 0; idx < rendererConfig.length; idx++) {
  //       const config = rendererConfig[idx];

  //       let images = [];
  //       if (Array.isArray(config.Image)) {
  //         images = config.Image;
  //       } else {
  //         images.push(config.Image);
  //       }

  //       const objectTemplate = {
  //         name: object.Name,
  //         id: object.Name + idx,
  //         tilingMode: config.TilingMode || "NONE",
  //         scale: config.Scale || 1.0,
  //         color: config.Color
  //           ? {
  //               r: config.Color[0],
  //               g: config.Color[1],
  //               b: config.Color[2],
  //             }
  //           : { r: 1, g: 1, b: 1 },
  //         zIdx: object.Z || 0,
  //         images,
  //       };

  //       this.objectTemplates[rendererName][objectTemplate.id] = objectTemplate;
  //       this.characterToObject[object.MapCharacter] = objectTemplate.id;
  //     }
  //   });
  // }

  }

  loadLevelString(levelString) {
    console.log("loading level string", levelString);
    

  }

  addTile() {}

  removeTile() {}

  toLevelString() {}
}

export default EditorState;
