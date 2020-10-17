Sokoban - 2
===========

Description
-------------

Push the boxes onto the marked spaces, once a box has moved onto a space, it cannot be moved

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
            - 8x7
     - .. thumbnail:: img/Sokoban_-_2-level-Sprite2D-0.png
     - .. thumbnail:: img/Sokoban_-_2-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 12x6
     - .. thumbnail:: img/Sokoban_-_2-level-Sprite2D-1.png
     - .. thumbnail:: img/Sokoban_-_2-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 7x8
     - .. thumbnail:: img/Sokoban_-_2-level-Sprite2D-2.png
     - .. thumbnail:: img/Sokoban_-_2-level-Block2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 6x7
     - .. thumbnail:: img/Sokoban_-_2-level-Sprite2D-3.png
     - .. thumbnail:: img/Sokoban_-_2-level-Block2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 8x12
     - .. thumbnail:: img/Sokoban_-_2-level-Sprite2D-4.png
     - .. thumbnail:: img/Sokoban_-_2-level-Block2D-4.png

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

       env = gym.make('GDY-Sokoban---2-v0')
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
           'Sokoban---2-Adv',
           'Single-Player/GVGAI/sokoban2.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
       )

       env = gym.make('GDY-Sokoban---2-Adv-v0')
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
     - box
     - box_in_place
     - wall
     - hole
     - avatar
   * - Map Char ->
     - b
     - f
     - w
     - h
     - A
   * - Sprite2D
     - .. image:: img/Sokoban_-_2-object-Sprite2D-box.png
     - .. image:: img/Sokoban_-_2-object-Sprite2D-box_in_place.png
     - .. image:: img/Sokoban_-_2-object-Sprite2D-wall.png
     - .. image:: img/Sokoban_-_2-object-Sprite2D-hole.png
     - .. image:: img/Sokoban_-_2-object-Sprite2D-avatar.png
   * - Block2D
     - .. image:: img/Sokoban_-_2-object-Block2D-box.png
     - .. image:: img/Sokoban_-_2-object-Block2D-box_in_place.png
     - .. image:: img/Sokoban_-_2-object-Block2D-wall.png
     - .. image:: img/Sokoban_-_2-object-Block2D-hole.png
     - .. image:: img/Sokoban_-_2-object-Block2D-avatar.png


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
     Name: Sokoban - 2
     Description: Push the boxes onto the marked spaces, once a box has moved onto a space, it cannot be moved
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/newset/floor2.png
     Player:
       AvatarObject: avatar # The player can only control a single avatar in the game
     Termination:
       Win:
         - eq: [box:count, 0] # If there are no boxes left
     Levels:
       - |
         wwwwwwww
         ww.....w
         ww.hbh.w
         ww.bAb.w
         w..hbh.w
         w......w
         wwwwwwww
       - |
         wwwwwwwwwwww
         w....www...w
         w.bb.....wAw
         w.b.whhh...w
         w...wwwwwwww
         wwwwwwwwwwww
       - |
         wwwwwww
         w.....w
         w.hbh.w
         w.bhb.w
         w.hbh.w
         w.bhb.w
         w..A..w
         wwwwwww
       - |
         wwwwww
         wh..ww
         wAbb.w
         ww...w
         www..w
         wwwwhw
         wwwwww
       - |
         wwwwwwww
         www.hhAw
         www.bb.w
         wwww.www
         wwww.www
         wwww.www
         wwww.www
         w....www
         w.w...ww
         w...w.ww
         www...ww
         wwwwwwww

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
             Object: [_empty, hole]
      
         # Boxes can move into empty space
         - Src:
             Object: box
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # The agent can push boxes
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: [box, box_in_place]
             Commands:
               - exec: 
                   Action: move

         # If a box is moved into a hole, it should change to in-place box
         - Src:
             Object: [box, box_in_place]
             Commands:
               - mov: _dest
               - change_to: box_in_place
               - reward: 1
           Dst:
             Object: hole
      
         # If in-place box is moved into empty space, it should be an empty box
         - Src:
             Object: box_in_place
             Commands:
               - mov: _dest
               - change_to: box
           Dst:
             Object: _empty

   Objects:
     - Name: box
       Z: 2
       MapCharacter: b
       Observers:
         Sprite2D:
           - Image: gvgai/newset/block2.png
         Block2D:
           - Shape: square
             Color: [1.0, 0.0, 0.0]
             Scale: 0.5

     - Name: box_in_place
       Z: 2
       MapCharacter: f
       Observers:
         Sprite2D:
           - Image: gvgai/newset/block1.png
         Block2D:
           - Shape: square
             Color: [0.0, 1.0, 0.0]
             Scale: 0.5

     - Name: wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
             Image:
               - gvgai/oryx/wall3_0.png
               - gvgai/oryx/wall3_1.png
               - gvgai/oryx/wall3_2.png
               - gvgai/oryx/wall3_3.png
               - gvgai/oryx/wall3_4.png
               - gvgai/oryx/wall3_5.png
               - gvgai/oryx/wall3_6.png
               - gvgai/oryx/wall3_7.png
               - gvgai/oryx/wall3_8.png
               - gvgai/oryx/wall3_9.png
               - gvgai/oryx/wall3_10.png
               - gvgai/oryx/wall3_11.png
               - gvgai/oryx/wall3_12.png
               - gvgai/oryx/wall3_13.png
               - gvgai/oryx/wall3_14.png
               - gvgai/oryx/wall3_15.png
         Block2D:
           - Shape: triangle
             Color: [0.6, 0.6, 0.6]
             Scale: 0.9

     - Name: hole
       Z: 1
       MapCharacter: h
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/cspell4.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 0.6

     - Name: avatar
       Z: 2
       MapCharacter: A
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/knight1.png
         Block2D:
           - Shape: triangle
             Color: [0.2, 0.2, 0.6]
             Scale: 1.0


