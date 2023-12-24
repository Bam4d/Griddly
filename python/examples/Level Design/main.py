from griddly import gd
from griddly.gym import GymWrapper
from griddly.util.render_tools import RenderToFile
from griddly.wrappers.render_wrapper import RenderWrapper

if __name__ == "__main__":
    env = GymWrapper(
        "levels.yaml",
        player_observer_type=gd.ObserverType.BLOCK_2D,
        global_observer_type=gd.ObserverType.BLOCK_2D,
    )

    global_obs_render_wrapper = RenderWrapper(env, "global", "rgb_array")
    player_1_obs_render_wrapper = RenderWrapper(env, 0, "rgb_array")
    player_2_obs_render_wrapper = RenderWrapper(env, 1, "rgb_array")

    image_renderer = RenderToFile()

    for i in range(3):
        env.reset(options={"level_id": i})
        obs = global_obs_render_wrapper.render()
        image_renderer.render(obs, f"level_{i}_global.png")
        if i > 0:
            obs = player_1_obs_render_wrapper.render()
            image_renderer.render(obs, f"level_{i}_player_1.png")
            obs = player_2_obs_render_wrapper.render()
            image_renderer.render(obs, f"level_{i}_player_2.png")

    level_string = \
""". c c c c c c . . c c c c c c . . c c . c1 c1 c1 c1 c1 c1 . . c2 c2 c2 c2 c2 c2 . . c c . . . . . . c c . . . . c c .
c c . . . . . . . c c . . . c c . c c . c1 c1 .  .  . c1 c1 . c2 c2 .  .  . c2 c2 . c c . . . . . . . c c . . c c . .
c c . . . c c c . c c c c c c . . c c . c1 c1 .  .  . c1 c1 . c2 c2 .  .  . c2 c2 . c c . . . . . . . . c c c c . . .
c c . . . . c c . c c . . . c c . c c . c1 c1 .  .  . c1 c1 . c2 c2 .  .  . c2 c2 . c c . . . . . . . . . c c . . . .
. c c c c c c . . c c . . . c c . c c . c1 c1 c1 c1 c1 c1 . . c2 c2 c2 c2 c2 c2 . . c c c c c c c . . . . c c . . . .
 """
    env.reset(options={"level_string": level_string})
    obs = global_obs_render_wrapper.render()
    image_renderer.render(obs, "custom_level_global.png")
    obs = player_1_obs_render_wrapper.render()
    image_renderer.render(obs, "custom_level_string_player_1.png")
    obs = player_2_obs_render_wrapper.render()
    image_renderer.render(obs, "custom_level_string_player_2.png")
