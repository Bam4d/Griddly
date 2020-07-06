Sokoban
=======

Description
-------------

Push the boxes into holes.

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
     - b
     - w
     - h
     - A
   * - SPRITE_2D
     - .. image:: img/Sokoban-object-SPRITE_2D-box.png
     - .. image:: img/Sokoban-object-SPRITE_2D-wall.png
     - .. image:: img/Sokoban-object-SPRITE_2D-hole.png
     - .. image:: img/Sokoban-object-SPRITE_2D-avatar.png
   * - BLOCK_2D
     - .. image:: img/Sokoban-object-BLOCK_2D-box.png
     - .. image:: img/Sokoban-object-BLOCK_2D-wall.png
     - .. image:: img/Sokoban-object-BLOCK_2D-hole.png
     - .. image:: img/Sokoban-object-BLOCK_2D-avatar.png


Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - SPRITE_2D
     - BLOCK_2D
   * - 0
     - .. thumbnail:: img/Sokoban-level-SPRITE_2D-0.png
     - .. thumbnail:: img/Sokoban-level-BLOCK_2D-0.png
   * - 1
     - .. thumbnail:: img/Sokoban-level-SPRITE_2D-1.png
     - .. thumbnail:: img/Sokoban-level-BLOCK_2D-1.png
   * - 2
     - .. thumbnail:: img/Sokoban-level-SPRITE_2D-2.png
     - .. thumbnail:: img/Sokoban-level-BLOCK_2D-2.png
   * - 3
     - .. thumbnail:: img/Sokoban-level-SPRITE_2D-3.png
     - .. thumbnail:: img/Sokoban-level-BLOCK_2D-3.png
   * - 4
     - .. thumbnail:: img/Sokoban-level-SPRITE_2D-4.png
     - .. thumbnail:: img/Sokoban-level-BLOCK_2D-4.png
   * - 5
     - .. thumbnail:: img/Sokoban-level-SPRITE_2D-5.png
     - .. thumbnail:: img/Sokoban-level-BLOCK_2D-5.png

YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Sokoban
     Description: Push the boxes into holes.
     TileSize: 24
     BackgroundTile: gvgai/newset/floor2.png
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
         w.whb.ww.bw
         w...b.....w
         wwwwwh....w
         ww........w
         ww..w..w.Aw
         ww..w..w.bw
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
           Image: gvgai/newset/block1.png
         Block2D:
           Shape: square
           Color: [1.0, 0.0, 0.0]
           Scale: 0.5

     - Name: wall
       MapCharacter: w
       Observers:
         Sprite2D:
           TilingMode: WALL_16
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
           Shape: triangle
           Color: [0.6, 0.6, 0.6]
           Scale: 0.9

     - Name: hole
       Z: 1
       MapCharacter: h
       Observers:
         Sprite2D:
           Image: gvgai/oryx/cspell4.png
         Block2D:
           Shape: square
           Color: [0.0, 1.0, 0.0]
           Scale: 0.6

     - Name: avatar
       Z: 2
       MapCharacter: A
       Observers:
         Sprite2D:
           Image: gvgai/oryx/knight1.png
         Block2D:
           Shape: triangle
           Color: [0.2, 0.2, 0.6]
           Scale: 1.0


