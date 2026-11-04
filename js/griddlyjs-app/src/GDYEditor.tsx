import { React, Component } from "react";
import Editor from "@monaco-editor/react";
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

    this.updateGDY = this.props.updateGDY;
    this.updateLevelString = this.props.updateLevelString;
    this.updateTrajectoryString = this.props.updateTrajectoryString;
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
          if (this.state.fileName === "GDY") {
            const updatedGDY = editor.getValue();
            this.updateGDY(updatedGDY);
          } else if (this.state.fileName === "Level") {
            const updatedLevelString = editor.getValue();
            this.updateLevelString(updatedLevelString);
          } else if (this.state.fileName === "Trajectory") {
            const updatedTrajectoryString = editor.getValue();
            this.updateTrajectoryString(updatedTrajectoryString);
          }
        }
      );

      this.editor.getModel().updateOptions({ tabSize: 2 });
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
          height="70vh"
          // options={{fontSize: "20"}}
          theme="vs-dark"
          onMount={this.handleEditorDidMount}
        />
      </>
    );
  }
}

export default GDYEditor;
