import { React, Component } from "react";
import Editor from "@monaco-editor/react";

class GDYEditor extends Component {
  constructor(props) {
    super(props);

    this.state = {
        
    };
  }

  render() {
    return (
      <Editor
        height="90vh"
        theme="vs-dark"
        defaultLanguage="yaml"
        defaultValue={this.props.gdyString}
      />
    );
  }
}

export default GDYEditor;
