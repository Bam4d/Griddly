import gym
import numpy as np

from griddly.util.rllib.environment.level_generator import LevelGenerator


class LabyrinthLevelGenerator(LevelGenerator):
    WALL = "w"
    AGENT = "A"
    GOAL = "x"
    EMPTY = "."

    def __init__(self, config):
        """
        Initialize the LabyrinthLevelGenerator.

        Parameters:
            config (dict): Configuration dictionary with the following optional keys:
                - width (int): Width of the maze (default: 9).
                - height (int): Height of the maze (default: 9).
                - wall_density (float): Wall density in the maze (default: 1).
                - num_goals (int): Number of goals to place in the maze (default: 1).

        Example usage:
        --------------
        config = {
            'width': 9,
            'height': 9,
            'wall_density': 1,
            'num_goals': 1
        }
        level_generator = LabyrinthLevelGenerator(config)
        level_string = level_generator.generate()
        env = gym.make('GDY-Maze-v0')
        env.reset(level_string=level_string)
        """

        super().__init__(config)
        self._width = config.get("width", 9)
        self._height = config.get("height", 9)
        self._wall_density = config.get(
            "wall_density", 1
        )  # Adjust this value to control wall density

        assert self._width % 2 == 1 and self._height % 2 == 1

        self._num_goals = config.get("num_goals", 1)

    def _generate_maze(self):
        """
        Generate the maze grid.

        Returns:
            np.ndarray: 2D array representing the maze with walls, empty spaces, and paths.

        Note: The Recursive Backtracking algorithm is used to generate the paths in the maze.
        """

        # Create a maze grid with walls
        maze = np.full(
            (self._width, self._height),
            LabyrinthLevelGenerator.WALL,
            dtype=np.dtype("U1"),
        )

        # Recursive Backtracking algorithm for generating maze paths
        def recursive_backtracking(x, y):
            maze[x, y] = LabyrinthLevelGenerator.EMPTY

            # Randomize the order of directions to explore
            directions = [(0, 1), (1, 0), (0, -1), (-1, 0)]
            np.random.shuffle(directions)

            for dx, dy in directions:
                nx, ny = x + 2 * dx, y + 2 * dy
                if (
                    0 <= nx < self._width
                    and 0 <= ny < self._height
                    and maze[nx, ny] == LabyrinthLevelGenerator.WALL
                ):
                    # Carve a path by removing walls
                    maze[nx - dx, ny - dy] = LabyrinthLevelGenerator.EMPTY
                    recursive_backtracking(nx, ny)

        # Start the Recursive Backtracking from a random position
        start_x, start_y = np.random.choice(
            range(1, self._width - 1), size=2
        ), np.random.choice(range(1, self._height - 1), size=2)
        recursive_backtracking(start_x[0], start_y[0])

        # Add more open spaces by removing walls randomly
        for x in range(1, self._width - 1):
            for y in range(1, self._height - 1):
                if (
                    maze[x, y] == LabyrinthLevelGenerator.WALL
                    and np.random.random() > self._wall_density
                ):
                    maze[x, y] = LabyrinthLevelGenerator.EMPTY

        return maze

    def _is_reachable(self, maze, x, y):
        """
        Check if a tile is reachable from the agent's starting position using a flood-fill algorithm.

        Parameters:
            maze (np.ndarray): 2D array representing the maze grid.
            x (int): X-coordinate of the tile to check.
            y (int): Y-coordinate of the tile to check.

        Returns:
            bool: True if the tile is reachable; False otherwise.
        """

        # Flood-fill algorithm to check if (x, y) is reachable from the agent's starting position
        stack = [(x, y)]
        visited = set()

        while stack:
            cx, cy = stack.pop()
            if (cx, cy) in visited:
                continue

            visited.add((cx, cy))
            for dx, dy in [(1, 0), (-1, 0), (0, 1), (0, -1)]:
                nx, ny = cx + dx, cy + dy
                if (
                    0 <= nx < self._width
                    and 0 <= ny < self._height
                    and maze[nx, ny] != LabyrinthLevelGenerator.WALL
                    and (nx, ny)
                    != (x, y)  # Exclude the goal position from the flood-fill
                ):
                    stack.append((nx, ny))

        return len(visited) == self._width * self._height - np.sum(
            maze == LabyrinthLevelGenerator.WALL
        )

    def _place_goals(self, maze, agent_x, agent_y):
        """
        Place the goals in the maze while ensuring they don't block the agent's access to all tiles.

        Parameters:
            maze (np.ndarray): 2D array representing the maze grid.
            agent_x (int): X-coordinate of the agent's starting position.
            agent_y (int): Y-coordinate of the agent's starting position.

        Returns:
            np.ndarray: Updated maze grid with goals placed.

        Note: The goals are placed in locations that do not block the agent's navigation to all tiles.
        """

        # Get all available empty spaces for goal placement
        available_spaces = np.transpose(np.where(maze == LabyrinthLevelGenerator.EMPTY))

        for _ in range(self._num_goals):
            np.random.shuffle(available_spaces)
            for goal_x, goal_y in available_spaces:
                # Check if the goal location does not block the agent's access to all tiles of the maze
                maze[goal_x, goal_y] = LabyrinthLevelGenerator.GOAL
                if self._is_reachable(maze, agent_x, agent_y):
                    break
                else:
                    maze[goal_x, goal_y] = LabyrinthLevelGenerator.EMPTY

        return maze

    def generate(self):
        """
        Generate a new maze level.

        Returns:
            str: String representation of the maze level.

        Example usage:
        --------------
        config = {
            'width': 9,
            'height': 9,
            'wall_density': 1,
            'num_goals': 1
        }
        level_generator = LabyrinthLevelGenerator(config)
        level_string = level_generator.generate()
        env = gym.make('GDY-Maze-v0')
        env.reset(level_string=level_string)
        """

        maze = self._generate_maze()

        # Place agent
        agent_x = 2 * np.random.randint(1, (self._width - 1) // 2)
        agent_y = 2 * np.random.randint(1, (self._height - 1) // 2)
        maze[agent_x, agent_y] = LabyrinthLevelGenerator.AGENT

        # Place goals with minimum distance constraint
        maze = self._place_goals(maze, agent_x, agent_y)

        level_string = "\n".join(["".join(row) for row in maze])

        return level_string


if __name__ == "__main__":
    import matplotlib.pyplot as plt

    env = gym.make("GDY-Labyrinth-v0")
    sizes = ["45x45", "21x21", "13x13"]

    for size in sizes:
        for i in range(3):
            config = {
                "width": int(size.split("x")[0]),
                "height": int(size.split("x")[1]),
            }

            level_generator = LabyrinthLevelGenerator(config)
            env.reset(level_string=level_generator.generate())

            obs = env.render(mode="rgb_array")
            plt.figure()
            plt.imshow(obs)
            plt.savefig(f"example_maze_{size}_{i+1}.png")
