import sys
import os

# The griddy lib is in the build directory when built so add it and then import
sys.path.extend([os.path.join(os.getcwd(), '../build')])

import griddy as gd


if __name__ == '__main__':

    print(gd.version)

    # Not gym Interface here...
    grid = gd.Grid(10,10)

    # Create a player
    player1 = grid.add_player('Bob')
    player2 = grid.add_player('Alice')

    # grid.add_object(player1, gd., 0, 0)

    #
    # # Add an object at a particular location, the object might be owned by a player
    # grid.add_object(player, 'OBJECT_TYPE', 0, 0)
    #
    # # Create an action belonging to a plyer
    # #action = player.create_action(...)
    #
    # # Create an observer
    # observer = gd.create_observer('thing')
    #
    # # Create a game from the previous settings
    # game = gd.create_game_process([player], observer, grid)
    #
    # game.perform_actions([action])
