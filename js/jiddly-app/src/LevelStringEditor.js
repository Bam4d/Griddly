import { React, Component } from "react";
import Editor from "@monaco-editor/react";
import yaml from "js-yaml";

class GDYEditor extends Component {
  constructor(props) {
    super(props);

    this.state = {};
  }

  handleEditorDidMount = (editor, monaco) => {

    if(editor) {
      this.editor = editor;

      this.editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS, () => {
        const levelString = editor.getValue();
        
        this.props.updateLevelString(levelString);
      });
    }
  };

  render() {
    return (
      <Editor
        height="90vh"
        theme="vs-dark"
        defaultLanguage="yaml"
        defaultValue="Loading........"
        value={this.props.levelString}
        onMount={this.handleEditorDidMount}
      />
    );
  }
}

export default GDYEditor;
