import React, { Component } from "react";
import "./App.scss";
import GriddlyJSCore from "./GriddlyJSCore";
import {
  Button,
  Container,
  Form,
  Nav,
  Navbar,
  Spinner,
} from "react-bootstrap";

// minigrid-style move mapping: 1=rotate left, 2=forward, 3=rotate right, 4=back
const KEY_TO_ACTION: Record<string, number> = {
  ArrowUp: 2,
  ArrowRight: 3,
  ArrowDown: 4,
  ArrowLeft: 1,
  " ": 5,
};

interface AppState {
  ready: boolean;
  loading: boolean;
  error: string | null;
  games: string[];
  game: string;
  level: number;
  levelCount: number;
  observer: string;
  observers: string[];
  fps: number;
}

class App extends Component<{}, AppState> {
  core: GriddlyJSCore;
  fpsFrames = 0;
  fpsStart = 0;

  constructor(props: {}) {
    super(props);
    this.core = new GriddlyJSCore();
    this.state = {
      ready: false,
      loading: false,
      error: null,
      games: [],
      game: "",
      level: 0,
      levelCount: 1,
      observer: "Sprite2D",
      observers: ["Sprite2D", "Block2D", "Isometric"],
      fps: 0,
    };
  }

  componentDidMount() {
    this.init();
    window.addEventListener("keydown", this.onKeyDown);
  }

  componentWillUnmount() {
    window.removeEventListener("keydown", this.onKeyDown);
  }

  init = async () => {
    try {
      await this.core.init();
      // SDL renders into <canvas id="canvas"> (WebGL/WebGPU in the browser)
      const games = this.core
        .listYamlFiles("/resources/games")
        .map((g) => g.replace("/resources/games/", ""))
        .sort();
      const defaultGame =
        games.find((g) => g.includes("minigrid-spiders")) || games[0];
      this.setState({ ready: true, games, game: defaultGame || "" }, () => {
        if (defaultGame) this.loadGame(defaultGame);
      });
    } catch (e: any) {
      this.setState({ error: String(e) });
    }
  };

  loadGame = async (game: string, level = 0, observer?: string) => {
    this.setState({ loading: true, error: null });
    try {
      // load the GDY first to discover which render observers it supports
      this.core.loadGDYFile("/resources/games/" + game);
      let observers = this.core.getAvailableRenderObservers();
      if (observers.length === 0) observers = ["Vector"];
      let obs = observer || this.state.observer;
      if (!observers.includes(obs)) obs = observers.includes("Sprite2D") ? "Sprite2D" : observers[0];
      this.core.startGame(obs, level);
      const lc: any = await Promise.resolve(this.core.getLevelCount());
      this.setState({ loading: false, game, level, observer: obs, observers, levelCount: typeof lc === "number" ? lc : 1 });
      this.drawFrame();
    } catch (e: any) {
      this.setState({ loading: false, error: this.core.getExceptionMessage(e) || String(e) });
    }
  };

  drawFrame = async () => {
    const canvas = document.getElementById("canvas") as HTMLCanvasElement | null;
    if (!canvas) return;
    try {
      const { pixels, width, height } = await this.core.observeImage();
      if (canvas.width !== width) canvas.width = width;
      if (canvas.height !== height) canvas.height = height;
      const ctx = canvas.getContext("2d");
      if (ctx) ctx.putImageData(new ImageData(new Uint8ClampedArray(pixels), width, height), 0, 0);
    } catch (e) {
      console.log("drawFrame error:", String(e));
    }
  };

  onKeyDown = (e: KeyboardEvent) => {
    const actionId = KEY_TO_ACTION[e.key];
    if (actionId === undefined || !this.state.ready || this.state.loading) return;
    e.preventDefault();
    try {
      const r = this.core.step([actionId]);
      const terminated = r && (r.terminated !== undefined ? r.terminated : r[1]);
      if (terminated) {
        // level complete/lost: reset to the same level automatically
        this.core.reset(this.state.level);
      }
      this.drawFrame(); // SDL rendered the frame; blit pixels to the 2D canvas
      this.fpsFrames++;
      const now = performance.now();
      if (this.fpsStart === 0) this.fpsStart = now;
      const elapsed = now - this.fpsStart;
      if (elapsed >= 1000) {
        this.setState({ fps: Math.round((this.fpsFrames * 1000) / elapsed) });
        this.fpsFrames = 0;
        this.fpsStart = now;
      }
    } catch (err: any) {
      this.setState({ error: this.core.getExceptionMessage(err) });
    }
  };

  render() {
    const { ready, loading, error, games, game, level, levelCount, observer, observers, fps } = this.state;
    return (
      <div className="app">
        <Navbar bg="dark" variant="dark" expand="lg">
          <Container fluid>
            <Navbar.Brand>Griddly 2.0</Navbar.Brand>
            <Nav>
              <Form.Select
                size="sm"
                style={{ width: 320 }}
                value={game}
                onChange={(e) => this.loadGame(e.target.value, 0)}
                disabled={!ready}
              >
                {games.map((g) => (
                  <option key={g} value={g}>
                    {g}
                  </option>
                ))}
              </Form.Select>
              <Form.Select
                size="sm"
                style={{ width: 130 }}
                value={observer}
                onChange={(e) =>
                  this.setState({ observer: e.target.value }, () =>
                    this.loadGame(this.state.game, this.state.level)
                  )
                }
                disabled={!ready}
              >
                {observers.map((o) => (
                  <option key={o}>{o}</option>
                ))}
              </Form.Select>
              <Button
                size="sm"
                variant="outline-light"
                disabled={level <= 0}
                onClick={() => this.loadGame(game, level - 1)}
              >
                ◀ Level
              </Button>
              <Form.Label className="text-light mb-0 mx-2">
                {level + 1}/{levelCount}
              </Form.Label>
              <Button
                size="sm"
                variant="outline-light"
                disabled={level + 1 >= levelCount}
                onClick={() => this.loadGame(game, level + 1)}
              >
                Level ▶
              </Button>
              <Button
                size="sm"
                variant="outline-light"
                onClick={() => this.core.reset(this.state.level)}
              >
                Reset
              </Button>
              <Form.Label className="text-light mb-0 ms-3">{fps} FPS</Form.Label>
            </Nav>
          </Container>
        </Navbar>

        <Container fluid className="play-area">
          {error && <div className="alert alert-danger m-3">{error}</div>}
          {loading && <Spinner animation="border" className="m-3" />}
          <div className="canvas-wrap">
            {/* SDL3 (emscripten port) renders into this canvas via WebGL/WebGPU */}
            <canvas id="canvas" />
            <canvas id="sdl-offscreen" style={{ display: "none" }} />
          </div>
          <div className="hint text-muted">
            Arrow keys / space = actions · pick a game and level above, then play
          </div>
        </Container>
      </div>
    );
  }
}

export default App;
