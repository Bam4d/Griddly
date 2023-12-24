from griddly import gd
from griddly.gym import GymWrapper
from griddly.util.render_tools import RenderToVideo
from griddly.wrappers.render_wrapper import RenderWrapper

if __name__ == "__main__":
    env = GymWrapper(
        "stochasticity.yaml",
        player_observer_type=gd.ObserverType.SPRITE_2D,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        level=0,
    )

    env.reset()

    global_obs_render_wrapper = RenderWrapper(env, "global", "rgb_array")
    global_recorder = RenderToVideo(global_obs_render_wrapper, "global_video_test.mp4")

    for i in range(1000):
        obs, reward, done, truncated, info = env.step(env.action_space.sample())

        env.render()
        global_recorder.capture_frame()

        if done:
            break

    global_recorder.close()
