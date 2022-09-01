import os

from griddly import GymWrapperFactory, gd, GymWrapper
from griddly.RenderTools import RenderToFile

if __name__ == "__main__":
    wrapper = GymWrapperFactory()

    name = "levels_env"

    current_path = os.path.dirname(os.path.realpath(__file__))

    env = GymWrapper(
        "levels.yaml",
        player_observer_type=gd.ObserverType.BLOCK_2D,
        global_observer_type=gd.ObserverType.BLOCK_2D,
    )

    image_renderer = RenderToFile()

    for i in range(3):
        env.reset(options={"level_id": i})
        obs = env.render(mode="rgb_array", observer="global")
        image_renderer.render(obs, f"level_{i}_global.png")
        if i > 0:
            obs = env.render(mode="rgb_array", observer=0)
            image_renderer.render(obs, f"level_{i}_player_1.png")
            obs = env.render(mode="rgb_array", observer=1)
            image_renderer.render(obs, f"level_{i}_player_2.png")

    level_string = """. c c c c c c . . c c c c c c . . c c . c c c c c c . . c c c c c c . . c c . . . . . . c c . . . . c c .  
c c . . . . . . . c c . . . c c . c c . c c . . . c c . c c . . . c c . c c . . . . . . . c c . . c c . .  
c c . . . c c c . c c c c c c . . c c . c c . . . c c . c c . . . c c . c c . . . . . . . . c c c c . . .  
c c . . . . c c . c c . . . c c . c c . c c . . . c c . c c . . . c c . c c . . . . . . . . . c c . . . .  
. c c c c c c . . c c . . . c c . c c . c c c c c c . . c c c c c c . . c c c c c c c . . . . c c . . . .
 """
    env.reset(options={"level_string": level_string})
    obs = env.render(mode="rgb_array", observer="global")
    image_renderer.render(obs, f"custom_level_global.png")
    obs = env.render(mode="rgb_array", observer=0)
    image_renderer.render(obs, f"custom_level_string_player_1.png")
    obs = env.render(mode="rgb_array", observer=1)
    image_renderer.render(obs, f"custom_level_string_player_2.png")
