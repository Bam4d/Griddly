.. _doc_partially_observable_zen_puzzle:

Partially Observable Zen Puzzle
===============================

.. code-block::

   Single-Player/GVGAI/zenpuzzle_partially_observable.yaml

Description
-------------

Set all the tiles in the level to the same color, but you cannot move over a tile more than once! (Not even sure why this is zen its super frustrating)

Levels
---------

.. list-table:: Levels
   :class: level-gallery
   :header-rows: 1

   * - 
     - Block2D
     - Vector
     - Sprite2D
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 12x12
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Block2D-0.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Vector-0.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Sprite2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 12x12
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Block2D-1.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Vector-1.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Sprite2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 12x12
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Block2D-2.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Vector-2.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Sprite2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 12x11
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Block2D-3.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Vector-3.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Sprite2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 12x12
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Block2D-4.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Vector-4.png
     - .. thumbnail:: img/Partially_Observable_Zen_Puzzle-level-Sprite2D-4.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Partially-Observable-Zen-Puzzle-v0')
       env.reset()
    
       # Replace with your own control algorithm!
       for s in range(1000):
           obs, reward, done, info = env.step(env.action_space.sample())
           env.render() # Renders the environment from the perspective of a single player

           env.render(observer='global') # Renders the entire environment
        
           if done:
               env.reset()


Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - avatar
     - ground
     - rock
   * - Map Char ->
     - `A`
     - `g`
     - `r`
   * - Block2D
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-avatar-Block2D.png
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-ground-Block2D.png
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-rock-Block2D.png
   * - Vector
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-avatar-Vector.png
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-ground-Vector.png
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-rock-Vector.png
   * - Sprite2D
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-avatar-Sprite2D.png
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-ground-Sprite2D.png
     - .. image:: img/Partially_Observable_Zen_Puzzle-tile-rock-Sprite2D.png


Actions
-------

move
^^^^

:Relative: The actions are calculated relative to the object being controlled.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - Rotate left
   * - 2
     - Move forwards
   * - 3
     - Rotate right


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Partially Observable Zen Puzzle
     Description: Set all the tiles in the level to the same color, but you cannot move over a tile more than once! (Not even sure why this is zen its super frustrating)
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/oryx/backBiege.png
     Player:
       Observer:
         RotateWithAvatar: true
         TrackAvatar: true
         Height: 5
         Width: 5
         OffsetX: 0
         OffsetY: 2
       AvatarObject: avatar
     Termination:
       Win:
         - eq: [ground:count, 0]
       Lose:
         - eq: [_steps, 1000]
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
       InputMapping:
         Inputs:
           1:
             Description: Rotate left
             OrientationVector: [-1, 0]
           2:
             Description: Move forwards
             OrientationVector: [0, -1]
             VectorToDest: [0, -1]
           3:
             Description: Rotate right
             OrientationVector: [1, 0]
         Relative: true
       Behaviours:

         # Avatar rotates
         - Src:
             Object: avatar
             Commands:
               - rot: _dir
           Dst:
             Object: avatar

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
               - reward: 1

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


