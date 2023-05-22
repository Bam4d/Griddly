import os

from griddly import GymWrapperFactory, gd, GymWrapper
from griddly.RenderTools import VideoRecorder

if __name__ == "__main__":
    wrapper = GymWrapperFactory()

    name = "stochasticity_env"

    current_path = os.path.dirname(os.path.realpath(__file__))

    env = GymWrapper(
        "stochasticity.yaml",
        player_observer_type=gd.ObserverType.SPRITE_2D,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        level=0,
    )

    env.reset()

    global_recorder = VideoRecorder()
    global_visualization = env.render(observer="global", mode="rgb_array")
    global_recorder.start("global_video_test.mp4", global_visualization.shape)

    for i in range(1000):

        obs, reward, done, truncated, info = env.step(env.action_space.sample())
        frame = env.render(observer="global", mode="rgb_array")

        global_recorder.add_frame(frame)

        if done:
            break

    global_recorder.close()
