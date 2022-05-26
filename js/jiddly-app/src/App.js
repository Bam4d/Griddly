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
  OverlayTrigger,
  Tooltip,
} from "react-bootstrap";

import {
  faFloppyDisk,
  faXmarkSquare,
  faExclamationTriangle,
  faInfoCircle,
  faFileCirclePlus,
  faClone,
  faTrashCan,
} from "@fortawesome/free-solid-svg-icons";
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";

import GDYEditor from "./GDYEditor";
import LevelEditorStateHandler from "./LevelEditorStateHandler";
import EditorHistory from "./EditorHistory";
import LevelSelector from "./renderer/level_selector/LevelSelector";
import { hashString } from "./Utils";

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
      levelSelector: {
        phaserWidth: 1000,
        phaserHeight: 120,
      },
      gdyHash: 0,
      gdyString: "",
      levelId: 0,
      rendererName: "",
      messages: {},
      selectedLevelId: 0,
      trajectories: [],
    };

    this.jiddly = new JiddlyCore();
    this.editorHistory = new EditorHistory(10);
    this.editorStateHandler = new LevelEditorStateHandler();

    this.newLevelString = `. . .
. . .
. . . 
`;
  }

  loadGDYURL = (url) => {
    return fetch(url).then((response) => JSON.parse(response.text()));
  };

  setCurrentLevel = (levelId) => {
    const levelString = this.state.gdy.Environment.Levels[levelId];
    this.editorStateHandler.loadLevelString(levelString);
    this.jiddly.reset(levelString);
    this.setState((state) => {
      return {
        ...state,
        selectedLevelId: levelId,
      };
    });
  };

  setEditorLevelString = (levelString) => {
    this.setState((state) => {
      return {
        ...state,
        levelString: levelString,
      };
    });
  };

  saveLevelString = (levelString, levelId) => {
    const gdy = this.state.gdy;

    let savedLevelId;

    // Overwrite a level, or just push a new one
    if (levelId) {
      gdy.Environment.Levels[levelId] = levelString;
      savedLevelId = levelId;
    } else {
      gdy.Environment.Levels.push(levelString);
      savedLevelId = gdy.Environment.Levels.length - 1;
    }

    const gdyString = yaml.dump(gdy);
    this.updateGDY(gdyString);

    return savedLevelId;
  };

  playLevel = (levelString) => {
    this.jiddly.reset(levelString);
  };

  saveNewLevel = () => {
    const savedLevelId = this.saveLevelString(this.state.levelString);
    this.jiddly.reset(this.state.levelString);
    this.setState((state) => {
      return {
        ...state,
        selectedLevelId: savedLevelId,
      };
    });
  };

  saveCurrentLevel = () => {
    const savedLevelId = this.saveLevelString(
      this.state.levelString,
      this.state.selectedLevelId
    );
    this.jiddly.reset(this.state.levelString);

    this.setState((state) => {
      return {
        ...state,
        selectedLevelId: savedLevelId,
      };
    });
  };

  newLevel = () => {
    this.editorStateHandler.loadLevelString(this.newLevelString);
    this.setKey("level");
    this.setState((state) => {
      return {
        ...state,
        selectedLevelId: -1,
      };
    });
  };

  deleteLevel = () => {
    const gdy = this.state.gdy;

    // Remove the level from the gdy
    gdy.Environment.Levels.splice(this.state.selectedLevelId, 1);

    const gdyString = yaml.dump(gdy);
    this.updateGDY(gdyString);
    this.setCurrentLevel(this.state.selectedLevelId - 1);
  };

  onTrajectoryComplete = (trajectoryBuffer) => {
    this.setState(state => {
      if(!(state.selectedLevelId in state.trajectories)) {
        state.trajectories[state.selectedLevelId] = [];
      }
      state.trajectories[state.selectedLevelId].push(trajectoryBuffer);
      this.editorHistory.updateState(this.state.gdy.Environment.Name, {trajectories: state.trajectories});

      return {
        ...state
      };
    });

  }

  findCompatibleRenderers = (observers, objects) => {
    const compatibleRenderers = new Map([
      [
        "Sprite2D",
        {
          Type: "SPRITE_2D",
        },
      ],
      [
        "Block2D",
        {
          Type: "BLOCK_2D",
        },
      ],
    ]);

    for (const [rendererName, config] of compatibleRenderers) {
      if (rendererName in observers) {
        compatibleRenderers.set(rendererName, {
          ...config,
          ...observers[rendererName],
        });
      }
    }

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

    for (const [rendererName, config] of compatibleRenderers) {
      if (!observersInObjects.has(rendererName)) {
        compatibleRenderers.delete(rendererName);
      }
    }

    return compatibleRenderers;
  };

  loadEditorState = async (editorState) => {
    try {
      const gdy = editorState.gdy;
      const trajectories = editorState.trajectories;
      const gdyString = yaml.dump(gdy);
      const trajectoriesString = yaml.dump(trajectories);

      this.editorStateHandler.onLevelString = this.setEditorLevelString;
      this.editorStateHandler.loadGDY(gdy);
      this.editorStateHandler.loadLevelString(
        gdy.Environment.Levels[gdy.Environment.Levels.length - 1]
      );

      const renderers = this.findCompatibleRenderers(
        gdy.Environment.Observers,
        gdy.Objects
      );

      if (renderers.size === 0) {
        this.displayMessage(
          "This GDY file does not contain any configurations for fully observable Sprite2D or Block2D renderers. We therefore don't know how to render this environment!",
          "error"
        );
        this.setState((state) => {
          return {
            ...state,
            gdyString,
          };
        });
        return;
      }

      const [rendererName] = renderers.keys();
      const rendererConfig = renderers.get(rendererName);

      return await this.jiddly
        .init()
        .then(() => {
          this.jiddly.loadGDY(gdyString);
          this.setState((state) => {
            return {
              ...state,
              gdyHash: hashString(gdyString),
              gdyString: gdyString,
              gdy: gdy,
              trajectories: trajectories,
              trajectoriesString: trajectoriesString,
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
              gdyHash: hashString(gdyString),
              gdyString: gdyString,
              gdy: gdy,
              trajectories: trajectories,
              trajectoriesString: trajectoriesString,
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
          gdyString: editorState.gdyString,
          trajectoriesString: editorState.trajectoriesString,
        };
      });
    }
  };

  updateGDY = (gdyString) => {
    const gdy = yaml.load(gdyString);
    this.editorHistory.updateState(gdy.Environment.Name, {gdy});
    try {
      this.jiddly.unloadGDY();
      this.jiddly.loadGDY(gdyString);
    } catch (e) {
      this.displayMessage("Unable to load GDY", e);
    }
    this.editorStateHandler.loadGDY(gdy);

    this.setState((state) => {
      return {
        ...state,
        gdyHash: hashString(gdyString),
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
        levelSelector: {
          phaserWidth: (2 * window.innerWidth) / 3,
          phaserHeight: 150,
        },
      };
    });
  };

  async componentDidMount() {
    this.updatePhaserCanvasSize();

    window.addEventListener("resize", this.updatePhaserCanvasSize, false);
    const editorState = this.editorHistory.getState("Grafter");

    if (!editorState) {
      await this.loadGDYURL(
        "resources/games/Single-Player/GVGAI/sokoban.yaml"
      ).then(gdy =>
        this.loadEditorState({ gdy, trajectories: [] })
      );
    } else {
      await this.loadEditorState(editorState);
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
    if (error) {
      console.log(error);
    }
    this.setState((state) => {
      const messageHash = hashString(content + type);
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
  };

  render() {
    return (
      <Container fluid className="jiddly-ide-container">
        <ToastContainer className="p-3" position="top-left">
          {Object.entries(this.state.messages).map(([key, message]) => {
            let icon;
            switch (message.type) {
              case "error":
                icon = (
                  <>
                    <FontAwesomeIcon
                      className="text-danger"
                      icon={faXmarkSquare}
                    />
                    <strong className="ms-2 me-auto">Error</strong>
                  </>
                );
                break;
              case "warning":
                icon = (
                  <>
                    <FontAwesomeIcon
                      className="text-warning"
                      icon={faExclamationTriangle}
                    />
                    <strong className="ms-2 me-auto"> Warning</strong>
                  </>
                );
                break;
              case "info":
                icon = (
                  <>
                    <FontAwesomeIcon
                      className="text-info"
                      icon={faInfoCircle}
                    />
                    <strong className="ms-2 me-auto"> Info</strong>
                  </>
                );
                break;
              default:
                break;
            }
            return (
              <Toast
                key={key}
                onClose={(e) => {
                  this.closeMessage(key);
                }}
              >
                <Toast.Header closeButton={true}>{icon}</Toast.Header>
                <Toast.Body>{message.content}</Toast.Body>
              </Toast>
            );
          })}
        </ToastContainer>
        <Row>
          <Col md={6}>
            <Tabs
              id="controlled-tab-example"
              activeKey={this.state.key}
              onSelect={(k) => this.setKey(k)}
              className="mb-3"
            >
              <Tab eventKey="play" title="Play">
                <Row>
                  <Col md={12}>
                    <div
                      ref={(tabPlayerContentElement) => {
                        this.tabPlayerContentElement = tabPlayerContentElement;
                      }}
                    >
                      <Player
                        gdyHash={this.state.gdyHash}
                        gdy={this.state.gdy}
                        trajectories={this.state.trajectories}
                        jiddly={this.state.jiddly}
                        rendererName={this.state.rendererName}
                        rendererConfig={this.state.rendererConfig}
                        height={this.state.levelPlayer.phaserHeight}
                        width={this.state.levelPlayer.phaserWidth}
                        selectedLevelId={this.state.selectedLevelId}
                        onTrajectoryComplete={this.onTrajectoryComplete}
                        onDisplayMessage={this.displayMessage}
                      ></Player>
                    </div>
                  </Col>
                </Row>
              </Tab>
              <Tab eventKey="level" title="Edit Levels">
                <Row>
                  <Col md={12}>
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
                </Row>
              </Tab>
            </Tabs>
          </Col>
          <Col md={6}>
            <GDYEditor
              gdyString={this.state.gdyString}
              levelString={this.state.levelString}
              trajectoryString={this.state.trajectoryString}
              updateGDY={this.updateGDY}
            />
          </Col>
        </Row>
        <Row>
          <Col md={2} className="button-panel"></Col>
          <Col md={6} className="button-panel">
            <OverlayTrigger
              placement="bottom"
              delay={{ show: 250, hide: 400 }}
              overlay={
                <Tooltip id="button-tooltip-2">Create a New Level</Tooltip>
              }
            >
              <Button variant="primary" size="sm" onClick={this.newLevel}>
                <FontAwesomeIcon icon={faFileCirclePlus} />
              </Button>
            </OverlayTrigger>
            <OverlayTrigger
              placement="bottom"
              delay={{ show: 250, hide: 400 }}
              overlay={
                <Tooltip id="button-tooltip-2">Save Current Level</Tooltip>
              }
            >
              <Button
                variant="secondary"
                size="sm"
                onClick={this.saveCurrentLevel}
                disabled={this.state.selectedLevelId === -1}
              >
                <FontAwesomeIcon icon={faFloppyDisk} />
              </Button>
            </OverlayTrigger>
            <OverlayTrigger
              placement="bottom"
              delay={{ show: 250, hide: 400 }}
              overlay={<Tooltip id="button-tooltip-2">Copy Level</Tooltip>}
            >
              <Button variant="secondary" size="sm" onClick={this.saveNewLevel}>
                <FontAwesomeIcon icon={faClone} />
              </Button>
            </OverlayTrigger>
            <OverlayTrigger
              placement="bottom"
              delay={{ show: 250, hide: 400 }}
              overlay={<Tooltip id="button-tooltip-2">Delete Level</Tooltip>}
            >
              <Button variant="danger" size="sm" onClick={this.deleteLevel}>
                <FontAwesomeIcon icon={faTrashCan} />
              </Button>
            </OverlayTrigger>
          </Col>
        </Row>
        <Row>
          <Col md={2} />
          <Col md={8}>
            <LevelSelector
              rendererConfig={this.state.rendererConfig}
              rendererName={this.state.rendererName}
              gdyHash={this.state.gdyHash}
              gdy={this.state.gdy}
              width={this.state.levelSelector.phaserWidth}
              height={this.state.levelSelector.phaserHeight}
              selectedLevelId={this.state.selectedLevelId}
              onSelectLevel={this.setCurrentLevel}
            />
          </Col>
          <Col md={2} />
        </Row>
      </Container>
    );
  }
}

export default App;
