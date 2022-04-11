// import logo from './logo.svg';
import React, { Component } from "react";
import "./App.scss";
import JiddlyCore from "./JiddlyCore";
import JiddlyRenderer from "./renderer/JiddlyRenderer";
import yaml from "js-yaml";

class App extends Component {
  constructor() {
    super();

    this.state = {
      envState: {},
    };

    this.jiddly = new JiddlyCore();
  }

  loadGDYURL = (url) => {
    return fetch(url).then((response) => response.text());
  };

  loadGDY = async (yamlString) => {
    this.gdy = yaml.load(yamlString);
    this.setState((state) => {
      return {
        ...state,
        gdy: this.gdy,
      };
    });

    return this.jiddly.init(yamlString);
  };

  setRendererState = async (envState) => {
    this.setState((state) => {
      return {
        ...state,
        envState,
      };
    });
  };

  async componentDidMount() {
    await this.loadGDYURL(
      "resources/games/Single-Player/GVGAI/spider-nest.yaml"
    )
      .then(this.loadGDY)
      .then(this.setRendererState);
  }

  envStep = async (action) => {
    await this.jiddly
      .envStep(action)
      .then(async (stepResults) => {
        if(stepResults.terminated) {
          await this.envReset();
        }
        return this.jiddly.getState();
      })
      .then(this.setRendererState);
  };

  envReset = async () => {
    await this.jiddly
      .envReset()
      .then((resetResults) => {
        
        return this.jiddly.getState();
      })
      .then(this.setRendererState);
  };

  render() {
    console.log("render:", this.state.envState);
    return (
      <JiddlyRenderer
        envState={this.state.envState}
        gdy={this.state.gdy}
        envStep={this.envStep}
        envReset={this.envReset}
      ></JiddlyRenderer>
    );
  }
}

export default App;
