// import logo from './logo.svg';
import yaml from "js-yaml";
import React, { Component } from "react";
import "./App.scss";
import GriddlyJSCore from "./GriddlyJSCore";
import Player from "./renderer/level_player/Player";
import PolicyDebugger from "./renderer/policy_debugger/PolicyDebugger";
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
  Nav,
  NavDropdown,
  NavItem,
} from "react-bootstrap";

import {
  faFloppyDisk,
  faXmarkSquare,
  faExclamationTriangle,
  faInfoCircle,
  faFileCirclePlus,
  faClone,
  faTrashCan,
  faBook,
} from "@fortawesome/free-solid-svg-icons";

import { faGithub, faDiscord } from "@fortawesome/free-brands-svg-icons";

import { FontAwesomeIcon } from "@fortawesome/react-fontawesome";

import GDYEditor from "./GDYEditor";
import LevelEditorStateHandler from "./LevelEditorStateHandler";
import EditorHistory from "./EditorHistory";
import LevelSelector from "./renderer/level_selector/LevelSelector";
import { hashString } from "./Utils";

import * as tf from "@tensorflow/tfjs";

class App extends Component {
  constructor() {
    super();

    this.state = {
      levelPlayer: {
        phaserWidth: 500,
        phaserHeight: 300,
      },
      levelEditor: {
        phaserWidth: 500,
        phaserHeight: 300,
      },
      policyDebugger: {
        phaserWidth: 500,
        phaserHeight: 300,
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
      projects: {
        envNames: [],
        templates: [],
      },
    };

    this.griddlyjs = new GriddlyJSCore();
    this.editorHistory = new EditorHistory(10);
    this.editorStateHandler = new LevelEditorStateHandler();

    this.newLevelString = `. . .
. . .
. . . 
`;
  }

  loadGDYURL = async (url) => {
    return fetch(url).then((response) => {
      return response.text().then((text) => yaml.load(text));
    });
  };

  setCurrentLevel = (levelId) => {
    this.setState((state) => {
      const levelString = state.gdy.Environment.Levels[levelId];

      try {
        this.editorStateHandler.loadLevelString(levelString, levelId);
      } catch (error) {
        this.displayMessage(
          "Unable to load level, please edit level string to fix any errors.",
          "error",
          error
        );
      }

      try {
        this.griddlyjs.reset(levelString);
      } catch (error) {
        this.displayMessage(
          "Unable to load level, please edit level string to fix any errors.",
          "error",
          error
        );
      }

      const trajectoryString = yaml.dump(state.trajectories[levelId]);
      return {
        ...state,
        selectedLevelId: levelId,
        levelString: levelString,
        trajectoryString: trajectoryString,
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

  updateLevelString = (levelString, levelId) => {
    const savedLevelId = this.saveLevelString(levelString, levelId);
    this.setState((state) => {
      return {
        ...state,
        selectedLevelId: savedLevelId,
      };
    });
  };

  updateTrajectoryString = (trajectoryString, levelId) => {
    this.setState((state) => {
      state.trajectories[state.selectedLevelId] = yaml.load(trajectoryString);
      this.editorHistory.updateState(this.state.gdy.Environment.Name, {
        trajectories: state.trajectories,
      });

      state.trajectoryString = trajectoryString;

      return {
        ...state,
      };
    });
  };

  playLevel = (levelString) => {
    this.griddlyjs.reset(levelString);
  };

  saveNewLevel = () => {
    const savedLevelId = this.saveLevelString(this.state.levelString);
    this.griddlyjs.reset(this.state.levelString);
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
    this.griddlyjs.reset(this.state.levelString);

    this.setState((state) => {
      return {
        ...state,
        selectedLevelId: savedLevelId,
      };
    });
  };

  newLevel = () => {
    this.editorStateHandler.loadLevelString(this.newLevelString, -1);
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
    this.setState((state) => {
      const trajectories = { ...state.trajectories };
      trajectories[state.selectedLevelId] = trajectoryBuffer;
      this.editorHistory.updateState(this.state.gdy.Environment.Name, {
        trajectories,
      });

      state.trajectoryString = yaml.dump(trajectoryBuffer);

      return {
        ...state,
        trajectories,
      };
    });
  };

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

  tryLoadModel = async (environmentName) => {
    return tf
      .loadGraphModel("./model/" + environmentName + "/model.json")
      .catch((error) => {
        console.log("Cannot load model for environment", environmentName);
      })
      .then((model) => {
        this.setState((state) => {
          return {
            ...state,
            model,
          };
        });
      });
  };

  tryLoadTrajectories = async (environmentName, trajectories) => {
    return fetch("./trajectories/" + environmentName + ".yaml")
      .then((response) => {
        return response.text().then((text) => {
          if (text.startsWith("<!")) {
            return [];
          } else {
            return yaml.load(text);
          }
        });
      })
      .then((preloadedTrajectories) => {
        for (const levelId in trajectories) {
          if (trajectories[levelId]) {
            preloadedTrajectories[levelId] = trajectories[levelId];
          }
        }

        this.setState((state) => {
          return {
            ...state,
            trajectories: preloadedTrajectories,
          };
        });
      })
      .catch((error) => {
        console.log(
          "Cannot load trajectories for environment",
          environmentName
        );
        this.setState((state) => {
          return {
            ...state,
            trajectories,
          };
        });
      });
  };

  loadEditorState = async (editorState) => {
    try {
      const gdy = editorState.gdy;

      const gdyString = yaml.dump(gdy);

      const lastLevelId = gdy.Environment.Levels.length - 1;
      const environmentName = gdy.Environment.Name;

      this.editorStateHandler.onLevelString = this.setEditorLevelString;

      this.tryLoadModel(environmentName);

      this.tryLoadTrajectories(environmentName, editorState.trajectories);

      try {
        this.updateGDY(gdyString);
        this.setCurrentLevel(lastLevelId);
      } catch (error) {
        this.displayMessage("Could not load GDY: " + error, "error");
        this.setState((state) => {
          return {
            ...state,
            gdyHash: hashString(gdyString),
            gdyString: gdyString,
            gdy: gdy,
            editorStateHandler: this.editorStateHandler,
            selectedLevelId: lastLevelId,
          };
        });
      }
    } catch (e) {
      this.displayMessage("Could not load GDY: " + e, "error");
      this.setState((state) => {
        return {
          ...state,
          gdyString: editorState.gdyString,
          trajectoryString: editorState.trajectoryString,
        };
      });
    }
  };

  loadRenderers = (gdy) => {
    const renderers = this.findCompatibleRenderers(
      gdy.Environment.Observers,
      gdy.Objects
    );

    if (renderers.size === 0) {
      this.displayMessage(
        "This GDY file does not contain any configurations for fully observable Sprite2D or Block2D renderers. We therefore don't know how to render this environment!",
        "error"
      );
    }

    const [rendererName] = renderers.keys();
    const rendererConfig = renderers.get(rendererName);

    this.setState((state) => {
      return {
        ...state,
        renderers: renderers,
        rendererName: rendererName,
        rendererConfig: rendererConfig,
      };
    });
  };

  updateGDY = (gdyString) => {
    const gdy = yaml.load(gdyString);
    this.editorHistory.updateState(gdy.Environment.Name, { gdy });
    try {
      this.griddlyjs.unloadGDY();
      this.griddlyjs.loadGDY(gdyString);
    } catch (e) {
      this.displayMessage("Unable to load GDY", e, "error");
    }
    this.editorStateHandler.loadGDY(gdy);

    this.loadRenderers(gdy);

    this.setState((state) => {
      return {
        ...state,
        gdyHash: hashString(gdyString),
        gdyString: gdyString,
        gdy: gdy,
        griddlyjs: this.griddlyjs,
        editorStateHandler: this.editorStateHandler,
      };
    });
  };

  updatePhaserCanvasSize = () => {
    this.setState((state) => {
      const width = Math.max(
        this.tabPlayerContentElement.offsetWidth,
        this.tabEditorContentElement.offsetWidth,
        this.tabDebuggerContentElement
          ? this.tabDebuggerContentElement.offsetWidth
          : 0
      );
      return {
        ...state,
        levelPlayer: {
          phaserWidth: width,
          phaserHeight: (6 * window.innerHeight) / 8,
        },
        levelEditor: {
          phaserWidth: width,
          phaserHeight: (6 * window.innerHeight) / 8,
        },
        policyDebugger: {
          phaserWidth: width,
          phaserHeight: (6 * window.innerHeight) / 8,
        },
        levelSelector: {
          phaserWidth: (2 * window.innerWidth) / 3,
          phaserHeight: 150,
        },
      };
    });
  };

  loadConfig = async () => {
    return fetch("config/config.json").then((response) => response.json());
  };

  refreshEnvList = () => {
    const envNames = this.editorHistory.getEnvList();
    this.setState((state) => {
      return {
        ...state,
        projects: {
          templates: state.projects.templates,
          envNames,
        },
      };
    });
  };

  setTemplates = (templates) => {
    this.setState((state) => {
      return {
        ...state,
        projects: {
          envNames: state.projects.envNames,
          templates,
        },
      };
    });
  };

  async componentDidMount() {
    window.addEventListener("resize", this.updatePhaserCanvasSize, false);

    this.updatePhaserCanvasSize();
    this.refreshEnvList();

    return await this.griddlyjs.init().then(() => {
      this.loadConfig().then((defaults) => {
        this.setTemplates(defaults.templates);
        const lastEnvName = this.editorHistory.getLastEnv();
        if (lastEnvName) {
          const editorState = this.editorHistory.getState(lastEnvName);
          this.loadEditorState(editorState);
        } else {
          this.loadGDYURL(defaults.defaultGDY).then((gdy) =>
            this.loadEditorState({ gdy })
          );
        }
      });
    });
  }

  setCurrentProject = (envName) => {
    const editorState = this.editorHistory.getState(envName);
    this.loadEditorState(editorState);
  };

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
      console.error(error);
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

  loadTemplate = async (template) => {
    await this.loadGDYURL(template.gdy).then((gdy) =>
      this.loadEditorState({ gdy })
    );
  };

  setCurrentEnv = (envName) => {
    const editorState = this.editorHistory.getState(envName);
    this.loadEditorState(editorState);
  };

  render() {
    return (
      <Container fluid className="griddlyjs-ide-container">
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
          <Col className="header-logo" md={6}>
            <Nav>
              <NavItem>
                <a href="https://griddly.ai">
                  <img
                    alt="Griddly Bear"
                    src="griddlybear192x192.png"
                    height="30"
                  />
                </a>
              </NavItem>

              <NavDropdown
                id="nav-dropdown-dark-example"
                title="New"
                menuVariant="dark"
              >
                <NavDropdown.Item onClick={() => this.createBlankProject()}>
                  Blank Project...
                </NavDropdown.Item>
                {this.state.projects.templates.length > 0 ? (
                  <>
                    <NavDropdown.Divider />
                    <NavDropdown.Header>Templates</NavDropdown.Header>
                    {this.state.projects.templates.map((template, key) => (
                      <NavDropdown.Item
                        key={key}
                        onClick={() => this.loadTemplate(template)}
                      >
                        {template.name}
                      </NavDropdown.Item>
                    ))}
                  </>
                ) : (
                  <></>
                )}
              </NavDropdown>
              {this.state.projects.envNames.length > 0 ? (
                <NavDropdown
                  id="nav-dropdown-dark-example"
                  title="Open"
                  menuVariant="dark"
                >
                  {this.state.projects.envNames.map((envName, key) => (
                    <NavDropdown.Item
                      key={key}
                      onClick={() => this.setCurrentEnv(envName)}
                    >
                      {envName}
                    </NavDropdown.Item>
                  ))}
                </NavDropdown>
              ) : (
                <></>
              )}
            </Nav>
          </Col>
          <Col className="header-navlinks" md={3}></Col>
          <Col className="header-navlinks" md={3}>
            <span className="navlink">
              <a
                target="_blank"
                rel="noreferrer"
                title="Docs"
                href="https://griddly.readthedocs.io/en/latest/"
              >
                <FontAwesomeIcon size="2x" icon={faBook} />
              </a>
            </span>
            <span className="navlink">
              <a
                target="_blank"
                rel="noreferrer"
                title="Github"
                href="https://github.com/Bam4d/Griddly"
              >
                <FontAwesomeIcon size="2x" icon={faGithub} />
              </a>
            </span>
            <span className="navlink">
              <a
                target="_blank"
                rel="noreferrer"
                title="Discord"
                href="https://discord.gg/xuR8Dsv"
              >
                <FontAwesomeIcon size="2x" icon={faDiscord} />
              </a>
            </span>
          </Col>
        </Row>
        <Row>
          <Col md={6}>
            <Tabs
              id="controlled-tab-example"
              activeKey={this.state.key}
              onSelect={(k, e) => {
                e.preventDefault();
                this.setKey(k);
              }}
              className="mb-3"
              transition={false}
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
                        trajectory={
                          this.state.trajectories[this.state.selectedLevelId]
                        }
                        griddlyjs={this.state.griddlyjs}
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
              {this.state.model ? (
                <Tab eventKey="debug" title="Debug Policies">
                  <Row>
                    <Col md={12}>
                      <div
                        ref={(tabDebuggerContentElement) => {
                          this.tabDebuggerContentElement =
                            tabDebuggerContentElement;
                        }}
                      >
                        <PolicyDebugger
                          gdyHash={this.state.gdyHash}
                          gdy={this.state.gdy}
                          griddlyjs={this.state.griddlyjs}
                          rendererName={this.state.rendererName}
                          rendererConfig={this.state.rendererConfig}
                          height={this.state.policyDebugger.phaserHeight}
                          width={this.state.policyDebugger.phaserWidth}
                          selectedLevelId={this.state.selectedLevelId}
                          onDisplayMessage={this.displayMessage}
                          model={this.state.model}
                        ></PolicyDebugger>
                      </div>
                    </Col>
                  </Row>
                </Tab>
              ) : (
                <></>
              )}
            </Tabs>
          </Col>
          <Col md={6}>
            <GDYEditor
              gdyString={this.state.gdyString}
              levelString={this.state.levelString}
              selectedLevelId={this.state.selectedLevelId}
              trajectoryString={this.state.trajectoryString}
              updateGDY={this.updateGDY}
              updateLevelString={this.updateLevelString}
              updateTrajectoryString={this.updateTrajectoryString}
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
              onDisplayMessage={this.displayMessage}
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
