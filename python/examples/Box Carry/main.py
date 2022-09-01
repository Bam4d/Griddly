from griddly import GymWrapper, gd

if __name__ == "__main__":
    env = GymWrapper(
        "box_carry.yaml",
        player_observer_type=gd.ObserverType.BLOCK_2D,
        global_observer_type=gd.ObserverType.BLOCK_2D,
    )

    env.reset(options={"level_id": 0})
    env.render()
    # Send the same actions to both agents
    env.step([3, 3])
    env.render()
    env.step([1, 1])
    env.render()
    env.step([2, 2])
    env.render()
    env.step([4, 4])
    env.render()
    env.step([4, 2])
    env.render()
    env.step([2, 4])
    env.render()
    env.step([3, 3])
    env.render()
    env.step([3, 3])
    env.render()
    env.step([3, 3])
    env.render()
