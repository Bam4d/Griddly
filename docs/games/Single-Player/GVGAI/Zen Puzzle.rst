Zen Puzzle
==========

Description
-------------

Set all the tiles in the level to the same color, but you cannot move over a tile more than once! (Not even sure why this is zen its super frustrating)

Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - SPRITE_2D
     - BLOCK_2D
   * - 0
     - .. thumbnail:: img/Zen_Puzzle-level-SPRITE_2D-0.png
     - .. thumbnail:: img/Zen_Puzzle-level-BLOCK_2D-0.png
   * - 1
     - .. thumbnail:: img/Zen_Puzzle-level-SPRITE_2D-1.png
     - .. thumbnail:: img/Zen_Puzzle-level-BLOCK_2D-1.png
   * - 2
     - .. thumbnail:: img/Zen_Puzzle-level-SPRITE_2D-2.png
     - .. thumbnail:: img/Zen_Puzzle-level-BLOCK_2D-2.png
   * - 3
     - .. thumbnail:: img/Zen_Puzzle-level-SPRITE_2D-3.png
     - .. thumbnail:: img/Zen_Puzzle-level-BLOCK_2D-3.png
   * - 4
     - .. thumbnail:: img/Zen_Puzzle-level-SPRITE_2D-4.png
     - .. thumbnail:: img/Zen_Puzzle-level-BLOCK_2D-4.png

Code Example
------------

Basic
^^^^^

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import numpy as np
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Zen-Puzzle-v0')
       env.reset()
    
       # Replace with your own control algorithm!
       for s in range(1000):
           obs, reward, done, info = env.step(env.action_space.sample())
           env.render()

           env.render(observer='global')


Advanced
^^^^^^^^

Create a customized Griddly Gym environment using the ``GymWrapperFactory``

.. code-block:: python


   import gym
   import numpy as np
   from griddly import GymWrapperFactory, gd

   if __name__ == '__main__':
       wrapper = GymWrapperFactory()

       wrapper.build_gym_from_yaml(
           'Zen-Puzzle-Adv',
           'Single-Player/GVGAI/zenpuzzle.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
           tile_size=10
       )

       env = gym.make('GDY-Zen-Puzzle-Adv-v0')
       env.reset()

       # Replace with your own control algorithm!
       for s in range(1000):
           obs, reward, done, info = env.step(env.action_space.sample())
           env.render()

           env.render(observer='global')


Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - avatar
     - ground
     - rock
   * - Map Char ->
     - A
     - g
     - r
   * - SPRITE_2D
     - .. image:: img/Zen_Puzzle-object-SPRITE_2D-avatar.png
     - .. image:: img/Zen_Puzzle-object-SPRITE_2D-ground.png
     - .. image:: img/Zen_Puzzle-object-SPRITE_2D-rock.png
   * - BLOCK_2D
     - .. image:: img/Zen_Puzzle-object-BLOCK_2D-avatar.png
     - .. image:: img/Zen_Puzzle-object-BLOCK_2D-ground.png
     - .. image:: img/Zen_Puzzle-object-BLOCK_2D-rock.png


Actions
-------

move
^^^^

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - Left
   * - 2
     - Up
   * - 3
     - Right
   * - 4
     - Down


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Zen Puzzle
     Description: Set all the tiles in the level to the same color, but you cannot move over a tile more than once! (Not even sure why this is zen its super frustrating)
     TileSize: 24
     BackgroundTile: gvgai/oryx/backBiege.png
     Player:
       AvatarObject: avatar
     Termination:
       Win:
         - eq: [ground:count, 0]
       Lose:
         - eq: [_max_steps, 1000]
     Levels:
       - |
         ............
         ............
         ............
         ...gggggg...
         ...gggggg...
         ...gggggg...
         .A.ggrrgg...
         ...gggggg...
         ...gggggg...
         ............
         ............
         ............
       - |
         ............
         ............
         ............
         ...gggggg...
         ...ggggrg...
         ...gggggg...
         .A.ggrggg...
         ...gggggg...
         ...gggggg...
         ............
         ............
         ............
       - | 
         ............
         ............
         ............
         ...gggggg...
         ...gggrgg...
         ...gggggg...
         .A.grgggg...
         ...ggggrg...
         ...gggggg...
         ............
         ............
         ............
       - |
         ............
         ............
         ............
         ...ggrrgg...
         ...rggggr...
         .A.ggrrgg...
         ...rggggr...
         ...ggrrgg...
         ............
         ............
         ............
       - | 
         ............
         ............
         ............
         ...rggggg...
         ...ggrgrg...
         ...rgggrg...
         .A.ggrggg...
         ...rgggrg...
         ...ggrggg...
         ............
         ............
         ............

   Actions:
     # Define the move action
     - Name: move
       Behaviours:
         # The agent can move around freely in empty space and over holes
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
                  
           Dst:
             Object: _empty

         - Src:
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: ground
             Commands:
               - change_to: walked 

   Objects:
     - Name: avatar
       MapCharacter: A
       Z: 1
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/angel1.png
         Block2D:
           - Shape: square
             Color: [0.8, 0.2, 0.2]
             Scale: 0.6

     - Name: ground
       MapCharacter: g
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/floorTileOrange.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.2, 0.4]
             Scale: 0.7

     - Name: walked
       Z: 0
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/floorTileGreen.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.6, 0.2]
             Scale: 0.8

     - Name: rock
       MapCharacter: r
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/wall5.png
         Block2D:
           - Shape: triangle
             Color: [0.2, 0.2, 0.2]
             Scale: 0.8


