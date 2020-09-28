Bait
====

Description
-------------

Get the key and unlock the door. Fill in the holes in the floor with blocks to get to the key.

Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - Sprite2D
     - Block2D
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 5x6
     - .. thumbnail:: img/Bait-level-Sprite2D-0.png
     - .. thumbnail:: img/Bait-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 13x9
     - .. thumbnail:: img/Bait-level-Sprite2D-1.png
     - .. thumbnail:: img/Bait-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 13x10
     - .. thumbnail:: img/Bait-level-Sprite2D-2.png
     - .. thumbnail:: img/Bait-level-Block2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 13x11
     - .. thumbnail:: img/Bait-level-Sprite2D-3.png
     - .. thumbnail:: img/Bait-level-Block2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 7x9
     - .. thumbnail:: img/Bait-level-Sprite2D-4.png
     - .. thumbnail:: img/Bait-level-Block2D-4.png

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

       env = gym.make('GDY-Bait-v0')
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
           'Bait-Adv',
           'Single-Player/GVGAI/bait.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
           tile_size=10
       )

       env = gym.make('GDY-Bait-Adv-v0')
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
     - hole
     - box
     - key
     - goal
     - mushroom
     - wall
   * - Map Char ->
     - A
     - 0
     - 1
     - k
     - g
     - m
     - w
   * - Sprite2D
     - .. image:: img/Bait-object-Sprite2D-avatar.png
     - .. image:: img/Bait-object-Sprite2D-hole.png
     - .. image:: img/Bait-object-Sprite2D-box.png
     - .. image:: img/Bait-object-Sprite2D-key.png
     - .. image:: img/Bait-object-Sprite2D-goal.png
     - .. image:: img/Bait-object-Sprite2D-mushroom.png
     - .. image:: img/Bait-object-Sprite2D-wall.png
   * - Block2D
     - .. image:: img/Bait-object-Block2D-avatar.png
     - .. image:: img/Bait-object-Block2D-hole.png
     - .. image:: img/Bait-object-Block2D-box.png
     - .. image:: img/Bait-object-Block2D-key.png
     - .. image:: img/Bait-object-Block2D-goal.png
     - .. image:: img/Bait-object-Block2D-mushroom.png
     - .. image:: img/Bait-object-Block2D-wall.png


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

   # http://www.mobygames.com/game/bait

   Version: "0.1"
   Environment:
     Name: Bait
     Description: Get the key and unlock the door. Fill in the holes in the floor with blocks to get to the key.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/oryx/backLBrown.png
     Player:
       AvatarObject: avatar
     Termination:
       Win:
         - eq: [goal:count, 0]
       Lose:
         - eq: [avatar:count, 0]
     Levels:
       - |
         w w w w w
         w g A w w
         w w . . w
         w . 1 1 w
         w w k . w
         w w w w w
       - |
         w w w w w w w w w w w w w
         w w w w w w g w w w w w w
         w w w w w . . . w w w w w
         w . . . w . A . w . . . w
         w . 1 . . . . . . . 1 . w
         w w w w w . 0 . w w w w w
         w w w w w w 0 w w w w w w
         w w w w w w k w w w w w w
         w w w w w w w w w w w w w
       - | 
         w w w w w w w w w w w w w
         w . . . 0 0 . 0 0 . . . w
         w . w 1 0 0 k 0 0 1 w . w
         w . w . 0 0 0 0 0 . w . w
         w . 1 . 0 0 m 0 0 . 1 . w
         w . w . w w 1 w w . w . w
         w . . . . . . . . . . . w
         w . w w w w 1 w w w w . w
         w . . . . . A g . . . . w
         w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w
         w A . . . . 1 0 0 0 1 g w
         w . 1 1 1 1 1 0 0 0 1 . w
         w 1 1 0 0 0 0 0 0 0 1 . w
         w 0 0 0 1 1 1 1 1 1 1 . w
         w 1 1 1 1 . . . . . . . w
         w . . . . . . 1 1 1 1 1 w
         w 1 1 1 1 1 1 1 0 0 0 1 w
         w m 0 0 0 0 0 0 0 0 0 0 w
         w 0 0 0 0 0 0 0 0 1 0 k w
         w w w w w w w w w w w w w
       - | 
         w w w w w w w
         w k w w w w w
         w 0 0 0 . . w
         w 0 m 0 1 . w
         w 0 1 1 1 . w
         w . 1 A 1 . w
         w 0 1 . 1 . w
         w w w w g . w
         w w w w w w w

   Actions:
     # Define the move action
     - Name: move
       Behaviours:
         # Avatar and boxes can move into empty space
         - Src:
             Object: [avatar, box]
             Commands:
               - mov: _dest
           Dst:
             Object: _empty
      
         # Boxes can be pushed by the avatar 
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: box
             Commands:
               - cascade: _dest

         # If a box falls into a hole, both disappear
         - Src:
             Object: box
             Commands:
               - remove: true
               - reward: 1
           Dst:
             Object: hole
             Commands:
               - remove: true

         # If the avatar falls into a hole remove the avatar
         - Src:
             Object: avatar
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: hole

         # If the avatar picks up a mushroom, remove the mushroom
         - Src:
             Object: avatar
             Commands:
               - reward: 1
               - mov: _dest
           Dst:
             Object: mushroom
             Commands: 
               - remove: true

         # Only an avatar with a key can 
         - Src:
             Preconditions:
               - eq: [has_key, 1]
             Object: avatar
             Commands:
               - reward: 5
           Dst:
             Object: goal
             Commands:
               - remove: true

         # Avatar picks up the key
         - Src: 
             Object: avatar
             Commands:
               - mov: _dest
               - incr: has_key
           Dst:
             Object: key
             Commands:
               - remove: true

   Objects:
     - Name: avatar
       MapCharacter: A
       Variables:
         - Name: has_key
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/swordman1_0.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 0.8

     - Name: hole
       MapCharacter: "0"
       Observers:
         Sprite2D:
           - Image: gvgai/newset/hole1.png
         Block2D:
           - Shape: square
             Color: [0.4, 0.4, 0.4]
             Scale: 0.7

     - Name: box
       MapCharacter: "1"
       Observers:
         Sprite2D:
           - Image: gvgai/newset/block3.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.6, 0.2]
             Scale: 0.8

     - Name: key
       MapCharacter: k
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/key2.png
         Block2D:
           - Shape: triangle
             Color: [0.8, 0.8, 0.2]
             Scale: 0.5

     - Name: goal
       MapCharacter: g
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/doorclosed1.png
         Block2D:
           - Shape: square
             Color: [0.0, 0.2, 1.0]
             Scale: 0.8

     - Name: mushroom
       MapCharacter: m
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/mushroom2.png
         Block2D:
           - Shape: square
             Color: [0.0, 0.8, 0.2]
             Scale: 0.5
  
     - Name: wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
             Image:
               - gvgai/oryx/dirtWall_0.png
               - gvgai/oryx/dirtWall_1.png
               - gvgai/oryx/dirtWall_2.png
               - gvgai/oryx/dirtWall_3.png
               - gvgai/oryx/dirtWall_4.png
               - gvgai/oryx/dirtWall_5.png
               - gvgai/oryx/dirtWall_6.png
               - gvgai/oryx/dirtWall_7.png
               - gvgai/oryx/dirtWall_8.png
               - gvgai/oryx/dirtWall_9.png
               - gvgai/oryx/dirtWall_10.png
               - gvgai/oryx/dirtWall_11.png
               - gvgai/oryx/dirtWall_12.png
               - gvgai/oryx/dirtWall_13.png
               - gvgai/oryx/dirtWall_14.png
               - gvgai/oryx/dirtWall_15.png
         Block2D:
           - Shape: square
             Color: [0.5, 0.5, 0.5]
             Scale: 0.9

