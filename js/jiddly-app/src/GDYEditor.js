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
        const updatedGDY = editor.getValue();
        
        // Also want to validate the GDY first?
        this.props.updateGDY(updatedGDY);
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
        value={this.props.gdyString}
        onMount={this.handleEditorDidMount}
      />
    );
  }
}

export default GDYEditor;
