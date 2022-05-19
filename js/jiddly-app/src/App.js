// import logo from './logo.svg';
import yaml from "js-yaml";
import React, { Component } from "react";
import "./App.scss";
import JiddlyCore from "./JiddlyCore";
import Player from "./renderer/level_player/Player";
import LevelEditor from "./renderer/level_editor/LevelEditor";
import {
  Col,
  Container,
  Row,
  Tabs,
  Tab,
  Button,
  ToastContainer,
  Toast,
} from "react-bootstrap";

import { faFloppyDisk, faFile, faXmarkSquare, faExclamationTriangle, faInfoCircle } from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";

import GDYEditor from "./GDYEditor";
import LevelStringEditor from "./LevelStringEditor";
import LevelEditorStateHandler from "./LevelEditorStateHandler";
import GDYHistory from "./GDYHistory";

class App extends Component {
  constructor() {
    super();

    this.state = {
      levelPlayer: {
        phaserWidth: 500,
        phaserHeight: 500,
      },
      levelEditor: {
        phaserWidth: 500,
        phaserHeight: 500,
      },
      gdyHash: 0,
      gdyString: "",
      levelId: 0,
      rendererName: "",
      messages: {},
    };

    this.jiddly = new JiddlyCore();
    this.gdyHistory = new GDYHistory(10);
    this.editorStateHandler = new LevelEditorStateHandler();

    this.newLevelString = `. . .
. . .
. . . 
`;
  }

  hashString = (string) => {
    let hash = 0,
      i,
      chr;
    if (string.length === 0) return hash;
    for (i = 0; i < string.length; i++) {
      chr = string.charCodeAt(i);
      hash = (hash << 5) - hash + chr;
      hash |= 0; // Convert to 32bit integer
    }
    return hash;
  };

  loadGDYURL = (url) => {
    return fetch(url).then((response) => response.text());
  };

  setCurrentLevelString = (levelString) => {
    this.setState((state) => {
      return {
        ...state,
        levelString: levelString,
      };
    });
  };

  saveLevelString = (levelString) => {
    const gdy = this.state.gdy;
    gdy.Environment.Levels.push(levelString);
    const gdyString = yaml.dump(gdy);
    this.setState((state) => {
      return {
        ...state,
        gdy,
        gdyHash: this.hashString(gdyString),
        gdyString,
      };
    });
  };

  playLevel = () => {};

  saveLevel = () => {
    this.saveLevelString(this.state.levelString);
    this.jiddly.reset(this.state.levelString);
  };

  newLevel = () => {
    this.editorStateHandler.loadLevelString(this.newLevelString);
  };

  loadLevelStringById = (levelId) => {
    this.editorStateHandler.loadLevelString(
      this.state.gdy.Environment.Levels[levelId]
    );
  };

  findCompatibleRenderers = (observers, objects) => {
    const compatibleRenderers = new Map([
      ["Sprite2D",{
        Type: "SPRITE_2D"
      }],
      ["Block2D", {
        Type: "BLOCK_2D"
      }]
    ]);

    // Search through observers for custom observer types
    for (const observerName in observers) {
      const observer = observers[observerName];

      // Ignore the default observers
      if (
        observerName !== "Sprite2D" &&
        observerName !== "Block2D" &&
        observerName !== "Entity" &&
        observerName !== "ASCII" &&
        observerName !== "Vector"
      ) {
        const observerType = observer.Type;

        // Only consider fully observable sprite and block observers
        if (observerType === "SPRITE_2D" || observerType === "BLOCK_2D") {
          if (
            !observer.Width &&
            !observer.Height &&
            !observer.OffsetX &&
            !observer.OffsetY &&
            !observer.Shader
          ) {
            compatibleRenderers.set(observerName, observer);
          }
        }
      }
    }

    const observersInObjects = new Set();

    // Search through objects for observer names
    for (const o in objects) {
      const object = objects[o];

      // Remove any observers that are missing definitions in objects and warn about them
      for (const observerName in object.Observers) {
        observersInObjects.add(observerName);
      }
    }

    for(const [rendererName, config] of compatibleRenderers) {
      if(!(observersInObjects.has(rendererName))) {
        compatibleRenderers.delete(rendererName)
      }
    }

    return compatibleRenderers;
  };

  loadGDY = async (yamlString) => {
    try {
      const gdy = yaml.load(yamlString);
      const gdyString = yaml.dump(gdy);

      this.editorStateHandler.onLevelString = this.setCurrentLevelString;
      this.editorStateHandler.loadGDY(gdy);
      this.editorStateHandler.loadLevelString(gdy.Environment.Levels[gdy.Environment.Levels.length-1]);

      const renderers = this.findCompatibleRenderers(gdy.Environment.Observers, gdy.Objects);

      if (renderers.size === 0) {
        this.displayMessage("This GDY file does not contain any configurations for fully observable Sprite2D or Block2D renderers. We therefore don't know how to render this environment!", "error");
        this.setState((state) => {
          return {
            ...state,
            gdyString: yamlString,
          };
        });
        return;
      } 

      const [rendererName] = renderers.keys();
      const rendererConfig = renderers.get(rendererName);

      return await this.jiddly
        .init()
        .then(() => {
          this.jiddly.loadGDY(yamlString);
          this.setState((state) => {
            return {
              ...state,
              gdyHash: this.hashString(gdyString),
              gdyString: gdyString,
              gdy: gdy,
              jiddly: this.jiddly,
              editorStateHandler: this.editorStateHandler,
              renderers: renderers,
              rendererName: rendererName,
              rendererConfig: rendererConfig,
            };
          });
        })
        .catch((reason) => {
          this.displayMessage("Could not load GDY: " + reason, "error");
          this.setState((state) => {
            return {
              ...state,
              gdyHash: this.hashString(gdyString),
              gdyString: gdyString,
              gdy: gdy,
              //jiddly: this.jiddly,
              editorStateHandler: this.editorStateHandler,
              renderers: renderers,
              rendererName: rendererName,
              rendererConfig: rendererConfig,
            };
          });
        });
    } catch (e) {
      this.displayMessage("Could not load GDY: " + e, "error");
      this.setState((state) => {
        return {
          ...state,
          gdyString: yamlString,
        };
      });
    }
  };

  updateGDY = (gdyString) => {
    const gdy = yaml.load(gdyString);
    this.gdyHistory.saveGDY(gdy.Environment.Name, gdyString);
    try {
      this.jiddly.unloadGDY();
      this.jiddly.loadGDY(gdyString);
    } catch (e) {
      this.displayMessage("Unable to load GDY", e);
    }
    this.editorStateHandler.loadLevelString(gdy.Environment.Levels[0]);
    this.editorStateHandler.loadGDY(gdy);

    this.setState((state) => {
      return {
        ...state,
        gdyHash: this.hashString(gdyString),
        gdyString: gdyString,
        gdy: gdy,
        jiddly: this.jiddly,
        editorStateHandler: this.editorStateHandler,
      };
    });
  };

  updatePhaserCanvasSize = () => {
    this.setState((state) => {
      const width = Math.max(
        this.tabEditorContentElement.offsetWidth,
        this.tabPlayerContentElement.offsetWidth
      );
      return {
        ...state,
        levelPlayer: {
          phaserWidth: width,
          phaserHeight: (4 * window.innerHeight) / 5,
        },
        levelEditor: {
          phaserWidth: width,
          phaserHeight: (4 * window.innerHeight) / 5,
        },
      };
    });
  };

  async componentDidMount() {
    this.updatePhaserCanvasSize();

    window.addEventListener("resize", this.updatePhaserCanvasSize, false);
    const currentGDY = this.gdyHistory.loadGDY("Grafter");
    if (!currentGDY) {
      await this.loadGDYURL(
        "resources/games/Single-Player/GVGAI/sokoban.yaml"
      ).then(this.loadGDY);
    } else {
      await this.loadGDY(currentGDY);
    }
  }

  setKey = (k) => {
    this.setState((state) => {
      return {
        ...state,
        key: k,
      };
    });

    this.updatePhaserCanvasSize();
  };

  displayMessage = (content, type, error) => {
    if(error) {
      console.log(error);
    }
    this.setState((state) => {
      const messageHash = this.hashString(content + type);
      state.messages[messageHash] = {
        content,
        type,
      };

      return {
        ...state,
      };
    });
  };

  closeMessage = (messageHash) => {
    this.setState((state) => {
      delete state.messages[messageHash];
      return {
        ...state,
      };
    });

  }

  render() {
    return (
      <Container fluid className="jiddly-ide-container">
        <ToastContainer className="p-3" position="top-left">
          {Object.entries(this.state.messages).map(([key, message]) => {
            let icon;
            switch (message.type) {
              case "error":
                icon = (<><FontAwesomeIcon className="text-danger" icon={faXmarkSquare}/><strong className="ms-2 me-auto">Error</strong></>)
                break;
              case "warning":
                icon = (<><FontAwesomeIcon className="text-warning" icon={faExclamationTriangle}/><strong className="ms-2 me-auto"> Warning</strong></>)
                break;
              case "info":
                icon = (<><FontAwesomeIcon className="text-info" icon={faInfoCircle}/><strong className="ms-2 me-auto"> Info</strong></>)
                break;
              default:
                break;
            }
            return (
              <Toast key={key} onClose={(e) => {this.closeMessage(key)}}>
                <Toast.Header closeButton={true}>{icon}</Toast.Header>
                <Toast.Body>{message.content}</Toast.Body>
              </Toast>
            );
          })}
        </ToastContainer>

        <Tabs
          id="controlled-tab-example"
          activeKey={this.state.key}
          onSelect={(k) => this.setKey(k)}
          className="mb-3"
        >
          <Tab eventKey="play" title="Play">
            <Row>
              <Col md={6}>
                <div
                  ref={(tabPlayerContentElement) => {
                    this.tabPlayerContentElement = tabPlayerContentElement;
                  }}
                >
                  <Player
                    gdyHash={this.state.gdyHash}
                    gdy={this.state.gdy}
                    jiddly={this.state.jiddly}
                    rendererName={this.state.rendererName}
                    rendererConfig={this.state.rendererConfig}
                    height={this.state.levelPlayer.phaserHeight}
                    width={this.state.levelPlayer.phaserWidth}
                    onDisplayMessage={this.displayMessage}
                  ></Player>
                </div>
              </Col>
              <Col md={6}>
                <GDYEditor
                  gdyString={this.state.gdyString}
                  updateGDY={this.updateGDY}
                />
              </Col>
            </Row>
          </Tab>
          <Tab eventKey="level" title="Edit Levels">
            <Row>
              <Col md={2} className="button-panel">
                <Button variant="primary" size="sm" onClick={this.saveLevel}>
                  <FontAwesomeIcon icon={faFloppyDisk} />
                </Button>
                <Button variant="primary" size="sm" onClick={this.newLevel}>
                  <FontAwesomeIcon icon={faFile} />
                </Button>
              </Col>
            </Row>
            <Row>
              <Col md={6}>
                <div
                  ref={(tabEditorContentElement) => {
                    this.tabEditorContentElement = tabEditorContentElement;
                  }}
                >
                  <LevelEditor
                    gdyHash={this.state.gdyHash}
                    gdy={this.state.gdy}
                    rendererName={this.state.rendererName}
                    rendererConfig={this.state.rendererConfig}
                    editorStateHandler={this.state.editorStateHandler}
                    height={this.state.levelEditor.phaserHeight}
                    width={this.state.levelEditor.phaserWidth}
                    onDisplayMessage={this.displayMessage}
                  ></LevelEditor>
                </div>
              </Col>
              <Col md={6}>
                <LevelStringEditor levelString={this.state.levelString} />
              </Col>
            </Row>
          </Tab>
        </Tabs>
      </Container>
    );
  }
}

export default App;
