// import logo from './logo.svg';
import React, { Component } from "react";
import "./App.scss";
import JiddlyCore from "./JiddlyCore";
import JiddlyRenderer from "./renderer/JiddlyRenderer";
import yaml from 'js-yaml'

class App extends Component {
  constructor() {
    super();

    this.state = {
      envState: {},
    }

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
        gdy:this.gdy,
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
    await this.loadGDYURL("resources/games/Single-Player/Mini-Grid/minigrid-doggo.yaml")
      .then(this.loadGDY)
      .then(this.setRendererState);
  }

  render() {
    console.log("render:", this.state.envState);
    return (
      <JiddlyRenderer envState={this.state.envState} gdy={this.state.gdy}></JiddlyRenderer>
    );
  }
}

export default App;
