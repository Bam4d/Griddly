from griddly import gd
from griddly.gym import GymWrapper
from griddly.util.render_tools import RenderToVideo
from griddly.wrappers.render_wrapper import RenderWrapper

if __name__ == "__main__":
    # Astar action space: 0: up, 1: down, 2: left, 3: right
    # env = GymWrapper('astar_opponent_environment.yaml',
    #                  player_observer_type=gd.ObserverType.VECTOR,
    #                  global_observer_type=gd.ObserverType.SPRITE_2D,
    #                  level=0)

    # Astar action space: 1: Rotate left, 2: Move forward, 3: Rotate right
    env = GymWrapper(
        "./astar_opponent_rotation_actions_environment.yaml",
        player_observer_type=gd.ObserverType.VECTOR,
        global_observer_type=gd.ObserverType.SPRITE_2D,
        level=1,
    )

    env.reset()
    global_render_wrapper = RenderWrapper(env, "global")
    video_render_wrapper = RenderWrapper(env, "global", "rgb_array")
    global_recorder = RenderToVideo(video_render_wrapper, "global_video_test.mp4")

    for i in range(1000):
        obs, reward, done, truncated, info = env.step(env.action_space.sample())

        global_render_wrapper.render()
        global_recorder.capture_frame()

        if done:
            env.reset()

    global_recorder.close()  # Finalise video
