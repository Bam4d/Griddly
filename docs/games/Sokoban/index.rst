Sokoban
=======

Description
-------------

Push the boxes into holes.

Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - Vector
     - Sprite2D
     - Block2D
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 13x9
     - .. thumbnail:: img/Sokoban-level-Vector-0.png
     - .. thumbnail:: img/Sokoban-level-Sprite2D-0.png
     - .. thumbnail:: img/Sokoban-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 13x9
     - .. thumbnail:: img/Sokoban-level-Vector-1.png
     - .. thumbnail:: img/Sokoban-level-Sprite2D-1.png
     - .. thumbnail:: img/Sokoban-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 13x9
     - .. thumbnail:: img/Sokoban-level-Vector-2.png
     - .. thumbnail:: img/Sokoban-level-Sprite2D-2.png
     - .. thumbnail:: img/Sokoban-level-Block2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 11x9
     - .. thumbnail:: img/Sokoban-level-Vector-3.png
     - .. thumbnail:: img/Sokoban-level-Sprite2D-3.png
     - .. thumbnail:: img/Sokoban-level-Block2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 7x7
     - .. thumbnail:: img/Sokoban-level-Vector-4.png
     - .. thumbnail:: img/Sokoban-level-Sprite2D-4.png
     - .. thumbnail:: img/Sokoban-level-Block2D-4.png
   * - .. list-table:: 

          * - Level ID
            - 5
          * - Size
            - 9x8
     - .. thumbnail:: img/Sokoban-level-Vector-5.png
     - .. thumbnail:: img/Sokoban-level-Sprite2D-5.png
     - .. thumbnail:: img/Sokoban-level-Block2D-5.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Sokoban-v0')
       env.reset()
    
       # Replace with your own control algorithm!
       for s in range(1000):
           obs, reward, done, info = env.step(env.action_space.sample())
           env.render() # Renders the environment from the perspective of a single player

           env.render(observer='global') # Renders the entire environment


Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - box
     - wall
     - hole
     - avatar
   * - Map Char ->
     - `b`
     - `w`
     - `h`
     - `A`
   * - Vector
     - .. image:: img/Sokoban-tile-box-Vector.png
     - .. image:: img/Sokoban-tile-wall-Vector.png
     - .. image:: img/Sokoban-tile-hole-Vector.png
     - .. image:: img/Sokoban-tile-avatar-Vector.png
   * - Sprite2D
     - .. image:: img/Sokoban-tile-box-Sprite2D.png
     - .. image:: img/Sokoban-tile-wall-Sprite2D.png
     - .. image:: img/Sokoban-tile-hole-Sprite2D.png
     - .. image:: img/Sokoban-tile-avatar-Sprite2D.png
   * - Block2D
     - .. image:: img/Sokoban-tile-box-Block2D.png
     - .. image:: img/Sokoban-tile-wall-Block2D.png
     - .. image:: img/Sokoban-tile-hole-Block2D.png
     - .. image:: img/Sokoban-tile-avatar-Block2D.png


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
     Name: Sokoban
     Description: Push the boxes into holes.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/newset/floor2.png
       Block2D:
         TileSize: 24
     Player:
       AvatarObject: avatar # The player can only control a single avatar in the game
     Termination:
       Win:
         - eq: [box:count, 0] # If there are no boxes left
     Levels:
       - |
         wwwwwwwwwwwww
         whhhhhhhhhhhw
         wh.........hw
         wh...b.b...hw
         wh....A....hw
         wh...b.b...hw
         wh.........hw
         whhhhhhhhhhhw
         wwwwwwwwwwwww
       - |
         wwwwwwwwwwwww
         w........w..w
         w...b.......w
         w...A.b.w.hww
         www.wb..wwwww
         w.......w.h.w
         w.b........ww
         w..........ww
         wwwwwwwwwwwww
       - |
         wwwwwwwwwwwww
         wwA........ww
         wwwwb......ww
         wwww.h......w
         wwww........w
         w..b...wbwwww
         w..h.......hw
         w...........w
         wwwwwwwwwwwww
       - |
         wwwwwwwwwww
         w...w.....w
         w.whb.wwb.w
         w...b.....w
         wwwwwh....w
         ww........w
         ww..w..wbAw
         ww..w..w..w
         wwwwwwwwwww
       - |
         wwwwwww
         w..hA.w
         w.whw.w
         w...b.w
         whbb.ww
         w..wwww
         wwwwwww
       - |
         wwwwwwwww
         ww.h....w
         ww...bA.w
         w....w..w
         wwwbw...w
         www...w.w
         wwwh....w
         wwwwwwwww

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
             Object: box
             Commands:
               - cascade: _dest

         # If a box is moved into a hole remove it
         - Src:
             Object: box
             Commands:
               - remove: true
               - reward: 1
           Dst:
             Object: hole

   Objects:
     - Name: box
       Z: 2
       MapCharacter: b
       Observers:
         Sprite2D:
           - Image: gvgai/newset/block1.png
         Block2D:
           - Shape: square
             Color: [1.0, 0.0, 0.0]
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
           - Shape: square
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


