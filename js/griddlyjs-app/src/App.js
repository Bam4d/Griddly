// import logo from './logo.svg';
import yaml, { YAMLException } from "js-yaml";
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
  Modal,
  Form,
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
import Intro from "./Intro";

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
        names: [],
        templates: {},
        blankTemplate: "",
      },
      newProject: {
        name: "",
        showModal: false,
        template: "",
      },
      projectName: "",
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

      const trajectoryString = yaml.dump(state.trajectories[levelId], {
        noRefs: true,
      });
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
    if (levelId && levelId >= 0) {
      gdy.Environment.Levels[levelId] = levelString;
      savedLevelId = levelId;
    } else {
      gdy.Environment.Levels.push(levelString);
      savedLevelId = gdy.Environment.Levels.length - 1;
    }

    const gdyString = yaml.dump(gdy, { noRefs: true });
    this.updateGDY(gdyString, this.state.projectName);

    return savedLevelId;
  };

  updateLevelString = (levelString) => {
    const savedLevelId = this.saveLevelString(
      levelString,
      this.state.selectedLevelId
    );
    this.editorStateHandler.loadLevelString(
      levelString,
      this.state.selectedLevelId
    );
    this.griddlyjs.reset(levelString);
    this.setState((state) => {
      return {
        ...state,
        selectedLevelId: savedLevelId,
      };
    });
  };

  updateTrajectoryString = (trajectoryString) => {
    this.setState((state) => {
      state.trajectories[state.selectedLevelId] = yaml.load(trajectoryString);
      this.editorHistory.updateState(state.projectName, {
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
    this.editorStateHandler.loadLevelString(
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

  saveCurrentLevel = () => {
    const savedLevelId = this.saveLevelString(
      this.state.levelString,
      this.state.selectedLevelId
    );
    this.editorStateHandler.loadLevelString(
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

    const gdyString = yaml.dump(gdy, { noRefs: true });
    this.updateGDY(gdyString, this.state.projectName);
    this.setCurrentLevel(this.state.selectedLevelId - 1);
  };

  onTrajectoryComplete = (trajectoryBuffer) => {
    this.setState((state) => {
      const trajectories = { ...state.trajectories };
      trajectories[state.selectedLevelId] = trajectoryBuffer;
      this.editorHistory.updateState(this.state.projectName, {
        trajectories,
      });

      state.trajectoryString = yaml.dump(trajectoryBuffer, { noRefs: true });

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
          TileSize: 24,
          RotateAvatarImage: true,
        },
      ],
      [
        "Block2D",
        {
          Type: "BLOCK_2D",
          TileSize: 24,
          RotateAvatarImage: true,
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
          if (text.startsWith("<!") || response.status !== 200) {
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

  loadProject = async (editorState, projectName) => {
    try {
      const gdy = editorState.gdy;

      const gdyString = yaml.dump(gdy, { noRefs: true });

      const lastLevelId = gdy.Environment.Levels.length - 1;

      this.editorStateHandler.onLevelString = this.setEditorLevelString;

      this.tryLoadModel(projectName);

      this.tryLoadTrajectories(projectName, editorState.trajectories);

      this.refreshProjectList();

      this.setState((state) => {
        return {
          ...state,
          loading: true,
        };
      });

      try {
        this.updateGDY(gdyString, projectName);
        this.setCurrentLevel(lastLevelId);
      } catch (e) {
        this.displayMessage("Could not load GDY", "error", e);
        this.setState((state) => {
          return {
            ...state,
            projectName,
            gdyHash: hashString(gdyString),
            gdyString: gdyString,
            gdy: gdy,
            editorStateHandler: this.editorStateHandler,
            selectedLevelId: lastLevelId,
          };
        });
      }
    } catch (e) {
      this.displayMessage("Could not load GDY", "error", e);
      this.setState((state) => {
        return {
          ...state,
          projectName,
          gdyString: editorState.gdyString,
          trajectoryString: editorState.trajectoryString,
        };
      });
    }
  };

  loadRenderers = (gdy) => {
    const renderers = this.findCompatibleRenderers(
      gdy.Environment.Observers || {},
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

  checkGriddlyJSCompatibility = (gdy) => {
    // Check for avatar object
    if (
      !("Player" in gdy.Environment) ||
      !("AvatarObject" in gdy.Environment.Player)
    ) {
      throw new Error(
        "Currently only Single-Player environments where an avatar is controlled by the agent are compatible with GriddlyJS. \n\n Perhaps you forgot to set the AvatarObject?"
      );
    }

    if (!("Levels" in gdy.Environment)) {
      throw new Error("Please define at least one level.");
    }
  };

  updateGDY = (gdyString, projectName) => {
    this.closeAllMessages();

    try {
      const gdy = yaml.load(gdyString);
      this.checkGriddlyJSCompatibility(gdy);
      this.editorHistory.updateState(projectName, { gdy });
      try {
        this.griddlyjs.unloadGDY();
        this.griddlyjs.loadGDY(gdyString);
      } catch (e) {
        this.displayMessage("Unable to load GDY", "error", e);
      }
      this.editorStateHandler.loadGDY(gdy);

      this.loadRenderers(gdy);

      this.setState((state) => {
        return {
          ...state,
          projectName,
          gdyHash: hashString(gdyString),
          gdyString: gdyString,
          gdy: gdy,
          griddlyjs: this.griddlyjs,
          editorStateHandler: this.editorStateHandler,
          loading: false,
        };
      });
    } catch (e) {
      this.setState((state) => {
        return {
          ...state,
          projectName,
          gdyHash: hashString(gdyString),
          gdyString: gdyString,
          loading: false,
        };
      });
      if (e instanceof YAMLException) {
        this.displayMessage(
          "There are syntax errors in your GDY: " + e.message,
          "error",
          e
        );
      } else {
        this.displayMessage("Unable to load GDY \n\n" + e.message, "error", e);
      }
    }
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

  refreshProjectList = () => {
    const projectNames = this.editorHistory.getProjectNames();

    this.setState((state) => {
      const newProjects = { ...this.state.projects, names: projectNames };
      return {
        ...state,
        projects: newProjects,
      };
    });
  };

  setTemplates = (templates, blankTemplate) => {
    this.setState((state) => {
      const newProjects = { ...this.state.projects, templates, blankTemplate };
      return {
        ...state,
        projects: newProjects,
      };
    });
  };

  async componentDidMount() {
    window.addEventListener("resize", this.updatePhaserCanvasSize, false);

    this.updatePhaserCanvasSize();
    this.refreshProjectList();

    return await this.griddlyjs.init().then(() => {
      this.loadConfig().then((defaults) => {
        this.setTemplates(defaults.templates, defaults.blankTemplate);
        const lastProjectName = this.editorHistory.getLastProject();
        if (lastProjectName) {
          const editorState = this.editorHistory.getState(lastProjectName);
          this.loadProject(editorState, lastProjectName);
        } else {
          this.loadGDYURL(defaults.defaultProject.gdy).then((gdy) => {
            this.loadProject({ gdy }, defaults.defaultProject.name);
            this.editorHistory.addProjectName(defaults.defaultProject.name);
          });
        }
      });
    });
  }

  setCurrentProject = (projectName) => {
    const editorState = this.editorHistory.getState(projectName);
    this.loadProject(editorState, projectName);
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

  createProjectFromTemplate = async (newProject) => {
    await this.loadGDYURL(newProject.template.gdy).then((gdy) => {
      this.loadProject({ gdy }, newProject.name);
      this.editorHistory.addProjectName(newProject.name);
    });
  };

  closeNewProjectModal = () => {
    this.setState((state) => {
      const newProject = { ...this.state.newProject, showModal: false };

      return {
        ...state,
        newProject,
      };
    });
  };

  newProjectModal = (template) => {
    this.setState((state) => {
      const newProject = {
        ...this.state.newProject,
        showModal: true,
        template,
      };

      return {
        ...state,
        newProject,
      };
    });
  };

  updateNewProjectName = (name) => {
    this.setState((state) => {
      const newProject = {
        ...this.state.newProject,
        showModal: true,
        name,
      };

      return {
        ...state,
        newProject,
      };
    });
  };

  createNewProject = (e) => {
    e.preventDefault();
    this.closeNewProjectModal();

    this.createProjectFromTemplate(this.state.newProject);
  };

  showIntroModal = () => {
    this.setState((state) => {
      return {
        ...state,
        showIntro: true,
      };
    });
  };

  closeIntroModal = () => {
    this.setState((state) => {
      return {
        ...state,
        showIntro: false,
      };
    });
  };

  closeAllMessages = () => {
    Object.entries(this.state.messages).map(([key, message]) => {
      this.closeMessage(key);
    });
  };

  render() {
    return (
      <Container fluid className="griddlyjs-ide-container">
        <Intro onClose={this.closeIntroModal} show={this.state.showIntro} />
        <Modal show={this.state.loading} backdrop="static">
          <Modal.Header>
            <Modal.Title>Loading Project.....</Modal.Title>
          </Modal.Header>
        </Modal>
        <Modal
          show={this.state.newProject.showModal}
          onHide={this.closeNewProjectModal}
        >
          <Modal.Header closeButton>
            <Modal.Title>New Project</Modal.Title>
          </Modal.Header>
          <Modal.Body>
            <Form onSubmit={this.createNewProject}>
              <Form.Group className="mb-3" controlId="projectName">
                <Form.Label>Name</Form.Label>
                <Form.Control
                  type="text"
                  placeholder="Griddly project..."
                  onChange={(e) => this.updateNewProjectName(e.target.value)}
                  autoFocus
                />
                <Form.Text className="text-muted">
                  Enter a unique name for your project.
                </Form.Text>
              </Form.Group>
            </Form>
          </Modal.Body>
          <Modal.Footer>
            <Button variant="secondary" onClick={this.closeNewProjectModal}>
              Cancel
            </Button>
            <Button variant="primary" onClick={this.createNewProject}>
              Create...
            </Button>
          </Modal.Footer>
        </Modal>
        <ToastContainer className="p-5" position="top-start">
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
                <Toast.Body className="message-text">
                  {message.content.split("\n").map(function (item, idx) {
                    return (
                      <span key={idx}>
                        {item}
                        <br />
                      </span>
                    );
                  })}
                </Toast.Body>
              </Toast>
            );
          })}
        </ToastContainer>
        <Row>
          <Col className="header-logo" md={4}>
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
                <NavDropdown.Item
                  onClick={() =>
                    this.newProjectModal(this.state.projects.blankTemplate)
                  }
                >
                  Blank Project...
                </NavDropdown.Item>
                {Object.keys(this.state.projects.templates).length > 0 ? (
                  <>
                    <NavDropdown.Divider />
                    <NavDropdown.Header>Templates</NavDropdown.Header>
                    {Object.keys(this.state.projects.templates).map((key) => (
                      <NavDropdown.Item
                        key={key}
                        onClick={() =>
                          this.newProjectModal(
                            this.state.projects.templates[key]
                          )
                        }
                      >
                        {key}
                      </NavDropdown.Item>
                    ))}
                  </>
                ) : (
                  <></>
                )}
              </NavDropdown>
              {Array.from(this.state.projects.names).length > 0 ? (
                <NavDropdown
                  id="nav-dropdown-dark-example"
                  title="Open"
                  menuVariant="dark"
                >
                  {Array.from(this.state.projects.names).map(
                    (projectName, key) => (
                      <NavDropdown.Item
                        key={key}
                        onClick={() => this.setCurrentProject(projectName)}
                      >
                        {projectName}
                      </NavDropdown.Item>
                    )
                  )}
                </NavDropdown>
              ) : (
                <></>
              )}
              <NavDropdown onClick={this.showIntroModal} title="Help" />
            </Nav>
          </Col>
          <Col className="header-project" md={4}>
            {this.state.projectName}
          </Col>
          <Col className="header-navlinks" md={4}>
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
              updateGDY={(gdy) => this.updateGDY(gdy, this.state.projectName)}
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
              >
                <FontAwesomeIcon icon={faFloppyDisk} />
              </Button>
            </OverlayTrigger>
            <OverlayTrigger
              placement="bottom"
              delay={{ show: 250, hide: 400 }}
              overlay={<Tooltip id="button-tooltip-2">Copy Level</Tooltip>}
            >
              <Button
                variant="secondary"
                size="sm"
                onClick={this.saveNewLevel}
                disabled={this.state.selectedLevelId === -1}
              >
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
