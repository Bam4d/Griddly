from timeit import default_timer as timer
import numpy as np
import gym

from griddly import GymWrapperFactory, gd
from griddly.RenderTools import VideoRecorder

if __name__ == "__main__":
    wrapper = GymWrapperFactory()

    name = "4rooms"

    wrapper.build_gym_from_yaml(
        name,
        "Single-Player/Mini-Grid/minigrid-drunkdwarf.yaml",
        player_observer_type=gd.ObserverType.SPRITE_2D,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        level=4,
    )

    env = gym.make(f"GDY-{name}-v0")

    env.reset()

    start = timer()

    frames = 0

    fps_samples = []

    player_recorder = VideoRecorder()
    player_visualization = env.render(mode="rgb_array")
    player_recorder.start("player_video_test.mp4", player_visualization.shape)

    global_recorder = VideoRecorder()
    global_visualization = env.render(observer="global", mode="rgb_array")
    global_recorder.start("global_video_test.mp4", global_visualization.shape)

    for s in range(100000):

        frames += 1
        obs, reward, done, info = env.step(env.action_space.sample())

        env.render()

        player_observation = env.render(mode="rgb_array")
        global_observation = env.render(observer="global", mode="rgb_array")

        player_recorder.add_frame(player_observation)
        global_recorder.add_frame(global_observation)

        if frames % 100 == 0:
            end = timer()
            fps = frames / (end - start)
            fps_samples.append(fps)
            print(f"fps: {fps}")
            frames = 0
            env.reset()
            start = timer()

        if done:
            env.reset()
    print(f"mean fps: {np.mean(fps_samples)}")
