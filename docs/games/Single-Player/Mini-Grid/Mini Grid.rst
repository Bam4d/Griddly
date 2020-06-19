Mini Grid
=========

Description
-------------

A port of the games provided in the https://github.com/maximecb/gym-minigrid environment.

Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - wall
     - goal
     - avatar
   * - Map Char ->
     - W
     - g
     - A
   * - SPRITE_2D
     - .. image:: img/Mini_Grid-object-SPRITE_2D-wall.png
     - .. image:: img/Mini_Grid-object-SPRITE_2D-goal.png
     - .. image:: img/Mini_Grid-object-SPRITE_2D-avatar.png
   * - BLOCK_2D
     - .. image:: img/Mini_Grid-object-BLOCK_2D-wall.png
     - .. image:: img/Mini_Grid-object-BLOCK_2D-goal.png
     - .. image:: img/Mini_Grid-object-BLOCK_2D-avatar.png


Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - SPRITE_2D
     - BLOCK_2D
   * - 0
     - .. thumbnail:: img/Mini_Grid-level-SPRITE_2D-0.png
     - .. thumbnail:: img/Mini_Grid-level-BLOCK_2D-0.png

YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Mini Grid
     Description: A port of the games provided in the https://github.com/maximecb/gym-minigrid environment.
     TileSize: 24
     BackgroundTile: gvgai/newset/floor2.png
     Player:
       Observer:
         RotateWithAvatar: true
         TrackAvatar: true
         Height: 7
         Width: 7
         OffsetX: 0
         OffsetY: 3
       Actions:
         DirectControl: avatar # The player can only control a single avatar in the game
         ControlScheme: DIRECT_RELATIVE
     Termination:
       Win:
         - eq: [goal:count, 0] # If there are no boxes left
     Levels:
       - |
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  A  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  W  W  W  W  .  W  W  W  W  W  W  W  W  .  W  W  W  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  g  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  W
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W

   Actions:
     # Define the move action
     - Name: move
       Behaviours:

         # Tell the agent to rotate if the avatar performs an action on itself
         - Src:
             Object: avatar
             Commands:
               - rot: _dir
           Dst:
             Object: avatar

         # The agent can move around freely in empty and always rotates the direction it is travelling
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # If the avatar moves into a goal object, the goal is removed, triggering a win condition
         - Src:
             Object: avatar
             Commands:
               - reward: 1
           Dst:
             Object: goal
             Commands:
               - remove: true

   Objects:
     - Name: wall
       MapCharacter: W
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
           Shape: square
           Color: [0.7, 0.7, 0.7]
           Scale: 1.0

     - Name: goal
       MapCharacter: g
       Observers:
         Sprite2D:
           Image: gvgai/oryx/cspell4.png
         Block2D:
           Shape: square
           Color: [0.0, 1.0, 0.0]
           Scale: 0.8

     - Name: avatar
       MapCharacter: A
       Observers:
         Sprite2D:
           Image: gvgai/oryx/knight1.png
         Block2D:
           Shape: triangle
           Color: [1.0, 0.0, 0.0]
           Scale: 1.0


