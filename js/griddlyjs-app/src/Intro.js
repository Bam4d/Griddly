import React, { Component } from "react";
import { Alert, Col, Container, Form, Modal, Nav, Row } from "react-bootstrap";

class Intro extends Component {
  constructor(props) {
    super(props);
    this.state = {
      show: false,
      slideIdx: 0,
      doNotShowOnStart: this.doNotShowOnStart()
    };


    

    this.slides = [
      {
        title: "Introduction",
        image: "intro/intro_updated.gif",
        content: (
          <div>
            <h5>Welcome to GriddlyJS</h5>
            <div>
              You can use GriddlyJS to create grid worlds for reinforcement
              learning.
            </div>
            <div>
              GriddlyJS helps you to build and test games mechanics, create
              levels, debug policies and even record human trajectories.
            </div>
          </div>
        ),
      },
      {
        title: "Managing projects",
        image: "intro/newproject_updated.gif",
        content: (
          <div>
            <div>
              <h5>Creating a new Project</h5>
              <ul>
                <li>
                  You can either create a blank project or start a project from
                  one of the templates provided.
                </li>
                <li>
                  If you choose to create a <b>blank project</b>, a very simple
                  environment with the most basic mechanics is loaded. This a
                  good starting point to understand how environments can be
                  configured using Griddly
                </li>
                <li>
                  For more complex projects, you can choose to start from an
                  existing template.
                </li>
              </ul>
              <div>
                <Alert variant="dark">
                  Tip: You can also create a blank template and then copy in
                  your own GDY text into the GDY editor.
                </Alert>
              </div>
            </div>
            <div>
              <h5>Loading an existing project</h5>
              <ul>
                <li>
                  All of the projects you create are automatically saved in your
                  browser. This includes any new levels trajectories and changes
                  to the GDY that you may have made.
                </li>
                <li>
                  You can select any of your existing projects in the{" "}
                  <b>Open</b> menu located at the top of the page
                </li>
              </ul>
            </div>
          </div>
        ),
      },
      {
        title: "Level Editor",
        image: "intro/leveleditor_updated.gif",
        content: (
          <div>
            <div>
              <h5>Editing Levels</h5>
              <ul>
                <li>
                  Use the level editor tab to <b>paint</b> new levels.
                </li>
                <li>
                  GriddlyJS reads the objects descriptions from the GDY
                  configuration, these objects can then be placed into the level
                  editor grid.
                </li>
                <li>
                  Modify the level text directly also. Press Ctrl+S to save the
                  changes back to the editor.
                </li>
                <li>
                  If you have a <code>BackgroundTile</code> configured, these
                  will be painted automatically to the bounds of the grid.
                </li>
                <li>
                  There's no limit to the dimensions of levels so you can be as
                  creative as you want!
                </li>
                <li>
                  As soon as you have created or modified your level, you can
                  click <b>play</b> to test the new level.
                </li>
              </ul>
              <div>
                <Alert variant="dark">
                  Tip: Use left click to paint and right click to remove items.
                </Alert>
              </div>
            </div>
            <div>
              <h5>Level Selection</h5>
              <ul>
                <li>
                  Click on the level thumbnails to choose a level to work with.
                </li>
                <li>
                  Create new levels, copy or modify existing levels and delete
                  them with the buttons just above the selection window.
                </li>
              </ul>
            </div>
          </div>
        ),
      },
      {
        title: "Debugging Mechanics",
        image: "intro/debuggingmechanics_updated.gif",
        content: (
          <div>
            <div>
              <h5>Making changes to GDY</h5>
              <ul>
                <li>
                  Change the mechanics of the game by modifying the GDY in the
                  editor. Press <b>Ctrl+S</b> to save changes.
                </li>
              </ul>
              <Alert variant="dark">
                Documentation, tutorials and examples of what can be done with
                GDY can be found
                <Alert.Link href="https://griddly.rtfd.io">here</Alert.Link>
              </Alert>

              <h5>Testing Mechanics and Debugging</h5>
              <ul>
                <li>
                  Press <b>P</b> to view the keyboard mapping for the
                  environment.
                </li>
                <li>
                  Press <b>I</b> to view the global variables.
                </li>
                <li>
                  Check that the behaviour of the variables and the actions are
                  what is expected in the environment.
                </li>
              </ul>
            </div>
          </div>
        ),
      },
      {
        title: "Recording Trajectories",
        image: "intro/recordingtrajectories_updated.gif",
        content: (
          <div>
            <div>
              <h5>Recording</h5>
              <ul>
                <li>
                  Click the <b>Record</b> button and then play the game, (the
                  actions mapped to the keyboard can be found by pressing
                  <b>P</b>)
                </li>
              </ul>
              <Alert variant="dark">
                Tip: If you make a mistake recording a trajectory you can click
                the red <b>stop</b> button to halt the recording.
              </Alert>
              <h5>Playback</h5>
              <ul>
                <li>
                  Once a trajectory is recorded, a green <b>play button</b> will
                  be enabled. Click this to playback your recorded trajectory
                </li>
              </ul>

              <Alert variant="dark">
                Tip: Copy the actions from the trajectory text view and load
                them in python using yaml, you can then pass them to the
                <code>env.step(...)</code> function to reproduce the trajectory
                for methods like behavioural cloning
              </Alert>
            </div>
          </div>
        ),
      },
      {
        title: "Debugging Policies",
        image: "intro/debugpolicies_updated.gif",
        content: (
          <div>
            <div>
              <h5>Policy Playback</h5>
              <ul>
                <li>
                  Click the <b>Play</b> button and then play the game, (the
                  actions mapped to the keyboard can be found by pressing
                  <b>P</b>)
                </li>
              </ul>
              <Alert variant="warning">
                Debugging policies is an advanced use case and is currently only available in the <b>Grafter Escape Rooms</b> environment.
                In order to add new policies, you will have to run GriddlyJS locally and add your own ONNX model to the <Alert.Link href="https://github.com/Bam4d/Griddly/tree/level_editor/js/griddlyjs-app/public/model">model directory</Alert.Link>
              </Alert>
            </div>
          </div>
        ),
      },
    ];
  }

  doNotShowOnStart = () => {
    return window.localStorage.getItem("_hideIntro") === "true";
  };

  setDoNotShowOnStart = (e) => {
    if(e.target.checked) {
      window.localStorage.setItem("_hideIntro", "true");
    } else {
      window.localStorage.setItem("_hideIntro", "false");
    }
    this.setState((state) => {
      return {
        ...state,
        doNotShowOnStart: e.target.checked
      }
    });
  };

  setShow = (show) => {
    if(!show) {
      this.props.onClose();
    }
    this.setState((state) => {
      return {
        ...state,
        show,
      };
    });
  };

  componentDidMount() {
    this.setState((state) => {
      return {
        ...state,
        show: !this.doNotShowOnStart(),
      };
    });
  }

  setSlide = (slideIdx) => {
    this.setState((state) => {
      return {
        ...state,
        slideIdx,
      };
    });
  };

  render() {
    const currentSlide = this.slides[this.state.slideIdx];
    return (
      <Modal
        show={this.state.show || this.props.show}
        onHide={() => this.setShow(false)}
        dialogClassName="modal-90w"
      >
        <Modal.Header closeButton>
          <Modal.Title>GriddlyJS Tutorial - {currentSlide.title}</Modal.Title>
        </Modal.Header>
        <Modal.Body>
          <Container>
            <Row>
              <Col md={12}>
                <Nav activeKey="0" onSelect={this.setSlide}>
                  {this.slides.map((slide, key) => {
                    return (
                      <Nav.Item>
                        <Nav.Link eventKey={key}>{slide.title}</Nav.Link>
                      </Nav.Item>
                    );
                  })}
                </Nav>
              </Col>
            </Row>
            <Row>
              <Col md={12} className="tutorial-video">
                <img
                  alt={currentSlide.title}
                  src={process.env.PUBLIC_URL + "/" + currentSlide.image}
                />
              </Col>
            </Row>
            <Row>
              <Col md={12} className="tutorial-content">
                {currentSlide.content}
              </Col>
            </Row>
            <Row>
              <Col md={12}>
                <hr />
                <Form.Check onChange={this.setDoNotShowOnStart} checked={this.state.doNotShowOnStart}/><small>Do not show again on startup.</small>
              </Col>
            </Row>
          </Container>
        </Modal.Body>
      </Modal>
    );
  }
}

export default Intro;
