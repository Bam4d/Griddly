import numpy as np
from timeit import default_timer as timer

from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderWindow

window = None

if __name__ == '__main__':

    renderWindow = RenderWindow(500, 500)

    loader = GriddlyLoader()

    grid = loader.load_game('RTS/basicRTS.yaml')
    grid.load_level(1)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)
    action_input_mappings = grid.get_action_input_mappings()
    action_definition_count = len(action_input_mappings)

    available_action_input_mappings = {}

    action_names = []
    for k, mapping in sorted(action_input_mappings.items()):
        if not mapping['Internal']:
            available_action_input_mappings[k] = mapping
            action_names.append(k)

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
    for i in range(0, 1000):
        for j in range(0, 1000):
            x = np.random.randint(grid.get_width())
            y = np.random.randint(grid.get_height())

            action_definition = np.random.randint(action_definition_count)
            action_name = action_names[action_definition]
            actionId = int(np.random.choice(list(action_input_mappings[action_name]["InputMappings"].keys())))

            # Alternate between player1 and player2 actions
            if j % 2 == 0:
                player1_step_result = player1.step(action_name, [x, y, actionId])
            else:
                player2_step_result = player2.step(action_name, [x, y, actionId])

            observation = np.array(game.observe(), copy=False)
            renderWindow.render(observation)

            frames += 1

            if frames % 1000 == 0:
                end = timer()
                print(f'fps: {frames / (end - start)}')
                frames = 0
                start = timer()

        game.reset()
