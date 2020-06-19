import numpy as np
from timeit import default_timer as timer

from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderWindow

window = None

if __name__ == '__main__':

    width = 30
    height = 30

    renderWindow = RenderWindow(32 * width, 32 * height)

    loader = GriddlyLoader()

    game_description = loader.load_game_description('RTS/basicRTS.yaml')

    grid = game_description.create_level(width, height)

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(-1, x, y, "minerals")

    for b in range(1, 3):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(b, x, y, "base")

    for i in range(0, 100):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(0, x, y, "movable_wall")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "harvester")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "puncher")

    for i in range(0, 10):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(1, x, y, "pusher")

    for i in range(0, 100):
        x = np.random.randint(width)
        y = np.random.randint(height)

        grid.add_object(0, x, y, "fixed_wall")

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    action_definition_count = grid.get_defined_actions_count()

    # Create a player
    player1 = game.register_player('Bob', gd.ObserverType.SPRITE_2D)
    player2 = game.register_player('Alice', gd.ObserverType.SPRITE_2D)

    game.init()

    game.reset()

    observation = np.array(game.observe(), copy=False)

    renderWindow.render(observation)

    start = timer()

    frames = 0

    # Player objects have the same interface as gym environments
    for i in range(0, 100000):
        for j in range(0, 10000):
            x = np.random.randint(width)
            y = np.random.randint(height)
            dir = np.random.randint(4)
            action_definition = np.random.randint(action_definition_count)
            action_name = grid.get_action_name(action_definition)

            # Alternate between player1 and player2 actions
            if j % 2 == 0:
                player1_step_result = player1.step(action_name, [x, y, dir])
            else:
                player2_step_result = player2.step(action_name, [x, y, dir])


            observation = np.array(game.observe(), copy=False)
            renderWindow.render(observation)

            frames += 1

            if frames % 1000 == 0:
                end = timer()
                print(f'fps: {frames / (end - start)}')
                frames = 0
                start = timer()

        game.reset()
