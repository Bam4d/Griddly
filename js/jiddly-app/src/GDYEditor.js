import { React, Component } from "react";
import Editor from "@monaco-editor/react";
import yaml from "js-yaml";
import { Nav } from "react-bootstrap";

class GDYEditor extends Component {
  constructor(props) {
    super(props);

    const editorModels = {
      GDY: {
        name: "GDY",
        language: "yaml",
        value: this.props.gdyString,
      },
      Level: {
        name: "Level",
        language: "text",
        value: this.props.levelString,
      },
      Trajectory: {
        name: "Trajectory",
        language: "yaml",
        value: this.props.trajectoryString,
      },
    };

    this.state = {
      fileName: "GDY",
      editorModels,
    };
  }

  static getDerivedStateFromProps(nextProps, prevState) {
    const editorModels = {
      GDY: {
        name: "GDY",
        language: "yaml",
        value: nextProps.gdyString,
      },
      Level: {
        name: "Level",
        language: "text",
        value: nextProps.levelString,
      },
      Trajectory: {
        name: "Trajectory",
        language: "yaml",
        value: nextProps.trajectoryString,
      },
    };

    return {
      ...prevState,
      editorModels,
    };
  }

  handleEditorDidMount = (editor, monaco) => {
    if (editor) {
      this.editor = editor;

      this.editor.addCommand(
        monaco.KeyMod.CtrlCmd | monaco.KeyCode.KeyS,
        () => {
          const updatedGDY = editor.getValue();

          // Also want to validate the GDY first?
          this.props.updateGDY(updatedGDY);
        }
      );
    }
  };

  changeModel = (fileName) => {
    this.setState((state) => {
      return {
        ...state,
        fileName,
      };
    });
  };

  render() {
    const file = this.state.editorModels[this.state.fileName];

    return (
      <>
        <Nav variant="tabs" defaultActiveKey="GDY">
          <Nav.Item>
            <Nav.Link key="GDY" onClick={() => this.changeModel("GDY")}>
              GDY
            </Nav.Link>
          </Nav.Item>
          <Nav.Item>
            <Nav.Link key="Level" onClick={() => this.changeModel("Level")}>
              Level
            </Nav.Link>
          </Nav.Item>
          <Nav.Item>
            <Nav.Link
              key="Trajectory"
              onClick={() => this.changeModel("Trajectory")}
            >
              Trajectory
            </Nav.Link>
          </Nav.Item>
        </Nav>
        <Editor
          path={file.name}
          value={file.value}
          language={file.language}
          height="80vh"
          theme="vs-dark"
          onMount={this.handleEditorDidMount}
        />
      </>
    );
  }
}

export default GDYEditor;
