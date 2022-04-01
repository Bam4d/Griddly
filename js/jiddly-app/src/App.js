// import logo from './logo.svg';
import React, { Component } from "react";
import "./App.scss";
import JiddlyCore from "./JiddlyCore";

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
    await this.loadGDYURL("gdy/spider-nest.yaml")
      .then(this.loadGDY)
      .then(this.setRendererState);
  }

  render() {
    console.log("render:", this.state.envState);
    return (
      // <div className="App">
      //   <header className="App-header">
      //     <img src={logo} className="App-logo" alt="logo" />
      //     <p>
      //       Edit <code>src/App.js</code> and save to reload.
      //     </p>
      //     <a
      //       className="App-link"
      //       href="https://reactjs.org"
      //       target="_blank"
      //       rel="noopener noreferrer"
      //     >
      <div>Learn React</div>
      //     </a>
      //   </header>
      // </div>
    );
  }
}

export default App;
