// import logo from './logo.svg';
import yaml from "js-yaml";
import React, { Component } from "react";
import "./App.scss";
import JiddlyCore from "./JiddlyCore";
import Player from "./renderer/level_player/Player";
import LevelEditor from "./renderer/level_editor/LevelEditor";
import { Col, Container, Row, Tabs, Tab, Button } from "react-bootstrap";

import { faFloppyDisk, faFile } from "@fortawesome/free-solid-svg-icons";
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
    };

    this.jiddly = new JiddlyCore();
    this.gdyHistory = new GDYHistory(10);
    this.editorStateHandler = new LevelEditorStateHandler();

    this.newLevelString = `. . .
. . .
. . . 
`;
  }

  hashGDYString = (gdyString) => {
    let hash = 0,
      i,
      chr;
    if (gdyString.length === 0) return hash;
    for (i = 0; i < gdyString.length; i++) {
      chr = gdyString.charCodeAt(i);
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
    const gdy = this.state.gdy.copy();
    gdy.Environment.Levels.push(levelString);
    const gdyString = yaml.dump(gdy);
    this.setState((state) => {
      return {
        ...state,
        gdy,
        gdyHash: this.hashGDYString(gdyString),
        gdyString,
      };
    });
  };

  saveLevel = () => {
    this.saveLevelString(this.state.levelString);
  };

  newLevel = () => {
    this.editorStateHandler.loadLevelString(this.newLevelString);
  };

  loadLevelStringById = (levelId) => {
    this.editorStateHandler.loadLevelString(
      this.state.gdy.Environment.Levels[levelId]
    );
  };

  loadGDY = async (yamlString) => {
    try {
      const gdy = yaml.load(yamlString);
      const gdyString = yaml.dump(gdy);

      this.editorStateHandler.onLevelString = this.setCurrentLevelString;
      this.editorStateHandler.loadGDY(gdy);
      this.editorStateHandler.loadLevelString(gdy.Environment.Levels[0]);

      return await this.jiddly
        .init()
        .then(() => {
          this.jiddly.loadGDY(yamlString);
        })
        .catch((reason) => {
          console.log(reason);
        })
        .finally(() => {
          this.setState((state) => {
            return {
              ...state,
              gdyHash: this.hashGDYString(gdyString),
              gdyString: gdyString,
              gdy: gdy,
              jiddly: this.jiddly,
              editorStateHandler: this.editorStateHandler,
            };
          });
        });
    } catch (e) {
      console.log(e);
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

    this.jiddly.unloadGDY();
    this.jiddly.loadGDY(gdyString);
    this.editorStateHandler.loadLevelString(gdy.Environment.Levels[0]);
    this.editorStateHandler.loadGDY(gdy);

    this.setState((state) => {
      return {
        ...state,
        gdyHash: this.hashGDYString(gdyString),
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
    const currentGDY = this.gdyHistory.loadGDY("Laser Tag");
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

  render() {
    return (
      <Container fluid className="jiddly-ide-container">
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
                  {/* <Player
                    gdyHash={this.state.gdyHash}
                    gdy={this.state.gdy}
                    jiddly={this.state.jiddly}
                    height={this.state.levelPlayer.phaserHeight}
                    width={this.state.levelPlayer.phaserWidth}
                  ></Player> */}
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
                    editorStateHandler={this.state.editorStateHandler}
                    height={this.state.levelEditor.phaserHeight}
                    width={this.state.levelEditor.phaserWidth}
                  ></LevelEditor>
                </div>
              </Col>
              <Col md={6}>
                <LevelStringEditor
                  levelString={this.state.levelString}
                />
              </Col>
            </Row>
          </Tab>
        </Tabs>
      </Container>
    );
  }
}

export default App;
