import os

from griddly import GymWrapperFactory, gd, GymWrapper
from griddly.RenderTools import VideoRecorder

if __name__ == "__main__":

    env = GymWrapper(
        "multi-obs.yaml",
        player_observer_type=["Player1Observer","Player2Observer"],
        global_observer_type=gd.ObserverType.SPRITE_2D,
        level=1,
    )

    env.reset()

    global_recorder = VideoRecorder()
    global_visualization = env.render(observer="global", mode="rgb_array")
    global_recorder.start("global_video_test.mp4", global_visualization.shape)

    for i in range(1000):

        obs, reward, done, info = env.step(env.action_space.sample())

        env.render(observer="global")
        env.render(observer=0)
        env.render(observer=1)
        frame = env.render(observer="global", mode="rgb_array")

        global_recorder.add_frame(frame)

        if done:
            env.reset()

    global_recorder.close()