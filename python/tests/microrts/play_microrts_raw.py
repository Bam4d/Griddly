from timeit import default_timer as timer
import numpy as np
import gym

from griddly import gd, GriddlyLoader
from griddly.RenderTools import VideoRecorder

if __name__ == '__main__':

    loader = GriddlyLoader()

    observer_type = gd.ObserverType.SPRITE_2D

    game_description = loader.load_game_description('RTS/basicRTS.yaml')

    grid = game_description.load_level(1)
    game = grid.create_game(observer_type)

    player1 = game.register_player(f'Player 1', observer_type)
    player2 = game.register_player(f'Player 2', observer_type)

    game.init()

    defined_action_count = grid.get_defined_actions_count()
    player_count = grid.get_player_count()

    start = timer()

    frames = 0

    fps_samples = []

    game.reset()

    grid_height = grid.get_height()
    grid_width = grid.get_width()

    player1_initial_observation = np.array(player1.observe(), copy=False)
    player2_initial_observation = np.array(player2.observe(), copy=False)
    global_initial_observation = np.array(game.observe(), copy=False)

    player1_recorder = VideoRecorder()
    player1_recorder.start("player1_raw_video_test.mp4", player1_initial_observation.shape)

    player2_recorder = VideoRecorder()
    player2_recorder.start("player2_raw_video_test.mp4", player2_initial_observation.shape)

    global_recorder = VideoRecorder()
    global_recorder.start("global_raw_video_test.mp4", global_initial_observation.shape)

    player1_recorder.add_frame(player1_initial_observation)
    player2_recorder.add_frame(player2_initial_observation)
    global_recorder.add_frame(global_initial_observation)

    for s in range(100):

        frames += 1

        player1_defined_action = np.random.randint(defined_action_count)
        player2_defined_action = np.random.randint(defined_action_count)

        player1_xy_action = np.random.randint(low=(0,0), high=(grid_width, grid_height), size=2)
        player2_xy_action = np.random.randint(low=(0,0), high=(grid_width, grid_height), size=2)

        player1_action_name = grid.get_action_name(player1_defined_action)
        player2_action_name = grid.get_action_name(player2_defined_action)

        player1_action = np.random.randint(6)
        player2_action = np.random.randint(6)

        player1_reward = player1.step(player1_action_name, [*player1_xy_action, player1_action])
        player2_reward = player2.step(player2_action_name, [*player1_xy_action, player2_action])

        player1_observation = np.array(player1.observe(), copy=False)
        player2_observation = np.array(player2.observe(), copy=False)
        global_observation = np.array(game.observe(), copy=False)

        player1_recorder.add_frame(player1_observation)
        player2_recorder.add_frame(player2_observation)
        global_recorder.add_frame(global_observation)

        if frames % 1000 == 0:
            end = timer()
            fps = (frames / (end - start))
            fps_samples.append(fps)
            print(f'fps: {fps}')
            frames = 0
            start = timer()
    print(f'mean fps: {np.mean(fps_samples)}')
