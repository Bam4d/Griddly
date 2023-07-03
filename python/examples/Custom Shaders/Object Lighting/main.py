from griddly import gd, GymWrapper
from griddly.util.render_tools import RenderToFile, RenderToVideo
from griddly.wrappers import RenderWrapper

if __name__ == "__main__":
    env = GymWrapper(
        "object_lighting.yaml",
        player_observer_type=gd.ObserverType.SPRITE_2D,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        level=0,
    )
    env.reset()

    global_obs_render_wrapper = RenderWrapper(env, "global", "rgb_array")
    player_obs_render_wrapper = RenderWrapper(env, 0, "rgb_array")

    reset_global_obs = global_obs_render_wrapper.render()
    reset_player_obs = player_obs_render_wrapper.render()

    render_to_file = RenderToFile()
    render_to_file.render(reset_global_obs, "reset_global.png")
    render_to_file.render(reset_player_obs, "reset_partial.png")

    global_recorder = RenderToVideo(global_obs_render_wrapper, "global_video_test.mp4")

    for i in range(1000):
        obs, reward, done, truncated, info = env.step(env.action_space.sample())

        env.render()
        global_recorder.capture_frame()

        if done:
            env.reset()

    global_recorder.close()
