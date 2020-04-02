import griddy as gd


if __name__ == '__main__':


    # Not gym Interface here...
    grid = gd.grid(10,10)

    # Create a player
    player = grid.create_player('My Player name')

    # Add an object at a particular location, the object might be owned by a player
    grid.add_object(player. 'OBJECT_TYPE', 0, 0)

    # Create an action belonging to a plyer
    action = player.create_action(...)

    # Create an observer
    observer = gd.observer('thing')

    # Create a game from the previous settings
    game = gd.create_game_process([player], observer, grid)

    game.perform_actions([action])
