import gym
from griddly import gd
from griddly.RenderTools import RenderWindow

if __name__ == '__main__':

    original_env = gym.make(f'GDY-Spiders-v0', level=4, global_observer_type=gd.ObserverType.SPRITE_2D, player_observer_type=gd.ObserverType.SPRITE_2D)
    original_env.enable_history(True)
    original_env.reset()

    env_clones = []
    clone_renderers = []

    # make 100 copies of the environment
    for e in range(3):
        env_clones.append(original_env.clone())

    for env in env_clones:
        global_observation = env.render(observer='global', mode='rgb_array')
        render_window = RenderWindow(300, 300)
        render_window.render(global_observation)
        clone_renderers.append(render_window)


    # step them 100 times with the same actions
    for s in range(10000):

        for i,env in enumerate(env_clones):
            obs, reward, done, info = env.step(env.action_space.sample())

            clone_renderers[i].render(obs)
