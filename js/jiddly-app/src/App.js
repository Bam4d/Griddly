// import logo from './logo.svg';
import yaml from "js-yaml";
import React, { Component } from "react";
import "./App.scss";
import JiddlyCore from "./JiddlyCore";
import Player from "./renderer/Player";
import LevelEditor from "./level_editor/LevelEditor";
import { Col, Container, Row, Tabs, Tab } from "react-bootstrap";

import GDYEditor from "./GDYEditor";

class App extends Component {
  constructor() {
    super();

    this.state = {
      phaserWidth: 500,
      phaserHeight: 500,
    };

    this.jiddly = new JiddlyCore();
  }

  loadGDYURL = (url) => {
    return fetch(url).then((response) => response.text());
  };

  loadGDY = async (yamlString) => {
    const gdyString = yamlString;
    const gdy = yaml.load(yamlString);

    return await this.jiddly.init(yamlString).then(() => {
      this.setState((state) => {
        return {
          ...state,
          gdyString: gdyString,
          gdy: gdy,
          jiddly: this.jiddly,
        };
      });
    });
  };


  updatePhaserCanvasSize = () => {
    this.setState((state) => {
      return {
        ...state,
        phaserWidth: this.tabContentElement.offsetWidth,
        phaserHeight: window.innerHeight / 2.0,
      };
    });
  }

  async componentDidMount() {
    this.updatePhaserCanvasSize()

    window.addEventListener("resize", this.updatePhaserCanvasSize, false);

    await this.loadGDYURL(
      "resources/games/Single-Player/GVGAI/sokoban.yaml"
    ).then(this.loadGDY);
  }

  setKey = (k) => {
    this.setState((state) => {
      return {
        ...state,
        key: k,
      };
    });
  };

  render() {
    return (
      <Container fluid className="jiddly-ide-container">
        <Row>
          <Col md={6}>
            <div
              ref={(tabContentElement) => {
                this.tabContentElement = tabContentElement;
              }}
            >
              <Tabs
                id="controlled-tab-example"
                activeKey={this.state.key}
                onSelect={(k) => this.setKey(k)}
                className="mb-3"
              >
                <Tab eventKey="play" title="Play">
                  <Player
                    gdy={this.state.gdy}
                    jiddly={this.state.jiddly}
                    height={this.state.phaserHeight}
                    width={this.state.phaserWidth}
                  ></Player>
                </Tab>
                <Tab eventKey="level" title="Edit Levels">
                  <LevelEditor
                    gdy={this.state.gdy}
                    height={this.state.phaserHeight}
                    width={this.state.phaserWidth}
                  ></LevelEditor>
                </Tab>
              </Tabs>
            </div>
          </Col>
          <Col md={6}>
            <GDYEditor gdyString={this.state.gdyString} />
          </Col>
        </Row>
      </Container>
    );
  }
}

export default App;
