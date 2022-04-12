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


    return await this.jiddly.init(yamlString).then((envState) => {
      this.setState((state) => {
        return {
          ...state,
          gdy: this.gdy,
          jiddly: this.jiddly,
        };
      });
    });
  };

  async componentDidMount() {
    await this.loadGDYURL(
      "resources/games/Single-Player/GVGAI/spider-nest.yaml"
    )
      .then(this.loadGDY)
      .then(this.setRendererState);
  }

  render() {
    return (
      <JiddlyRenderer
        gdy={this.state.gdy}
        jiddly={this.state.jiddly}
      ></JiddlyRenderer>
    );
  }
}

export default App;
