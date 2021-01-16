Clusters
========

Description
-------------

Cluster the coloured objects together by pushing them against the static coloured blocks.

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
            - 13x10
     - .. thumbnail:: img/Clusters-level-Vector-0.png
     - .. thumbnail:: img/Clusters-level-Sprite2D-0.png
     - .. thumbnail:: img/Clusters-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 13x10
     - .. thumbnail:: img/Clusters-level-Vector-1.png
     - .. thumbnail:: img/Clusters-level-Sprite2D-1.png
     - .. thumbnail:: img/Clusters-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 13x10
     - .. thumbnail:: img/Clusters-level-Vector-2.png
     - .. thumbnail:: img/Clusters-level-Sprite2D-2.png
     - .. thumbnail:: img/Clusters-level-Block2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 13x10
     - .. thumbnail:: img/Clusters-level-Vector-3.png
     - .. thumbnail:: img/Clusters-level-Sprite2D-3.png
     - .. thumbnail:: img/Clusters-level-Block2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 13x10
     - .. thumbnail:: img/Clusters-level-Vector-4.png
     - .. thumbnail:: img/Clusters-level-Sprite2D-4.png
     - .. thumbnail:: img/Clusters-level-Block2D-4.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Clusters-v0')
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
     - avatar
     - wall
     - spike
     - red_box
     - red_block
     - green_box
     - green_block
     - blue_box
     - blue_block
   * - Map Char ->
     - `A`
     - `w`
     - `h`
     - `2`
     - `b`
     - `3`
     - `c`
     - `1`
     - `a`
   * - Vector
     - .. image:: img/Clusters-tile-avatar-Vector.png
     - .. image:: img/Clusters-tile-wall-Vector.png
     - .. image:: img/Clusters-tile-spike-Vector.png
     - .. image:: img/Clusters-tile-red_box-Vector.png
     - .. image:: img/Clusters-tile-red_block-Vector.png
     - .. image:: img/Clusters-tile-green_box-Vector.png
     - .. image:: img/Clusters-tile-green_block-Vector.png
     - .. image:: img/Clusters-tile-blue_box-Vector.png
     - .. image:: img/Clusters-tile-blue_block-Vector.png
   * - Sprite2D
     - .. image:: img/Clusters-tile-avatar-Sprite2D.png
     - .. image:: img/Clusters-tile-wall-Sprite2D.png
     - .. image:: img/Clusters-tile-spike-Sprite2D.png
     - .. image:: img/Clusters-tile-red_box-Sprite2D.png
     - .. image:: img/Clusters-tile-red_block-Sprite2D.png
     - .. image:: img/Clusters-tile-green_box-Sprite2D.png
     - .. image:: img/Clusters-tile-green_block-Sprite2D.png
     - .. image:: img/Clusters-tile-blue_box-Sprite2D.png
     - .. image:: img/Clusters-tile-blue_block-Sprite2D.png
   * - Block2D
     - .. image:: img/Clusters-tile-avatar-Block2D.png
     - .. image:: img/Clusters-tile-wall-Block2D.png
     - .. image:: img/Clusters-tile-spike-Block2D.png
     - .. image:: img/Clusters-tile-red_box-Block2D.png
     - .. image:: img/Clusters-tile-red_block-Block2D.png
     - .. image:: img/Clusters-tile-green_box-Block2D.png
     - .. image:: img/Clusters-tile-green_block-Block2D.png
     - .. image:: img/Clusters-tile-blue_box-Block2D.png
     - .. image:: img/Clusters-tile-blue_block-Block2D.png


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


box_counter
^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - The only action here is to increment the box count


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Clusters
     Description: Cluster the coloured objects together by pushing them against the static coloured blocks.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: oryx/oryx_fantasy/floor1-2.png
     Variables:
       - Name: box_count
         InitialValue: 0
     Player:
       AvatarObject: avatar # The player can only control a single avatar in the game
     Termination:
       Win:
         - eq: [box_count, 0]
       Lose:
         - eq: [broken_box:count, 1]
         - eq: [avatar:count, 0]
     Levels:
       - |
         w w w w w w w w w w w w w
         w . . . . . . . . . . . w
         w . . 1 1 . . . 2 . 2 . w
         w . . . . 1 . . . . . . w
         w . . . a . . . . . 2 . w
         w . . . . . . . h . . . w
         w . . . . 1 . . . . b . w
         w . . . . . . 1 . . . . w
         w . . . . . . . . A . . w
         w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w
         w . . . . . . . . . . . w
         w . . 1 . . 2 . c 3 . . w
         w . . . . h . . h . . . w
         w . . . 2 . . 3 . . 1 . w
         w . . . . b . . h . . . w
         w . . 3 . . . 2 . . 1 . w
         w . . h . h . . . a . . w
         w . . . . . A . . . . . w
         w w w w w w w w w w w w w
       - | 
         w w w w w w w w w w w w w
         w . . a . . b . . c . . w
         w . . . . . . . . . . . w
         w . . . . . . . . . . . w
         w h h h h h . h h h h h w
         w . . . . h . h . . . . w
         w . 1 2 . h . h . 1 3 . w
         w . 3 . . . . . . . 2 . w
         w . . . . . A . . . . . w
         w w w w w w w w w w w w w
       - | 
         w w w w w w w w w w w w w
         w . . . . . . . . . . . w
         w . . . 1 . 2 . . c . . w
         w . . . . . 3 . . 3 . . w
         w . . a . 2 . . . h . . w
         w . . . . h h . 3 . . . w
         w . . 1 . . . . . 2 . . w
         w . . . . . 1 . . b . . w
         w . . . . . A . . . . . w
         w w w w w w w w w w w w w
       - | 
         w w w w w w w w w w w w w
         w . . . . . . . . . . . w
         w . . . . . . 1 . . . . w
         w . . h . . b . . h . . w
         w . . . . 1 . . . . . . w
         w . . 3 . . . . 2 . . . w
         w . . . a . h . . c . . w
         w . . . . 3 . . . . 2 . w
         w . . . . . A . . . . . w
         w w w w w w w w w w w w w

   Actions:

     # A simple action to count the number of boxes in the game at the start
     # Not currently a way to do complex things in termination conditions like combine multiple conditions
     - Name: box_counter
       InputMapping:
         Internal: true
         Inputs:
           1: 
             Description: "The only action here is to increment the box count"
       Behaviours:
         - Src: 
             Object: [blue_box, red_box, green_box]
             Commands: 
               - incr: box_count
           Dst:
             Object: [blue_box, red_box, green_box]

     # Define the move action
     - Name: move
       Behaviours:

         # Avatar and boxes can move into empty space
         - Src:
             Object: [avatar, blue_box, green_box, red_box]
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
             Object: [blue_box, green_box, red_box]
             Commands:
               - cascade: _dest

         # When boxes are pushed against the blocks they change
         - Src:
             Object: blue_box
             Commands:
               - change_to: blue_block
               - reward: 1
               - decr:  box_count
           Dst:
             Object: blue_block
         - Src:
             Object: red_box
             Commands:
               - reward: 1
               - change_to: red_block
               - decr:  box_count
           Dst:
             Object: red_block
         - Src:
             Object: green_box
             Commands:
               - reward: 1
               - change_to: green_block
               - decr:  box_count
           Dst:
             Object: green_block

         # Boxes break if they hit the spikes  
         - Src:
             Object: [blue_box, green_box, red_box]
             Commands:
               - change_to: broken_box
               - reward: -1
           Dst:
             Object: spike

         # Avatar dies if it hits the spikes
         - Src:
             Object: avatar
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: spike

   Objects:
     - Name: avatar
       MapCharacter: A
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/knight1.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 0.8

     - Name: wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
             Image:
               - oryx/oryx_fantasy/wall1-0.png
               - oryx/oryx_fantasy/wall1-1.png
               - oryx/oryx_fantasy/wall1-2.png
               - oryx/oryx_fantasy/wall1-3.png
               - oryx/oryx_fantasy/wall1-4.png
               - oryx/oryx_fantasy/wall1-5.png
               - oryx/oryx_fantasy/wall1-6.png
               - oryx/oryx_fantasy/wall1-7.png
               - oryx/oryx_fantasy/wall1-8.png
               - oryx/oryx_fantasy/wall1-9.png
               - oryx/oryx_fantasy/wall1-10.png
               - oryx/oryx_fantasy/wall1-11.png
               - oryx/oryx_fantasy/wall1-12.png
               - oryx/oryx_fantasy/wall1-13.png
               - oryx/oryx_fantasy/wall1-14.png
               - oryx/oryx_fantasy/wall1-15.png
         Block2D:
           - Shape: square
             Color: [0.5, 0.5, 0.5]
             Scale: 0.9

     - Name: spike
       MapCharacter: h
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/spike2.png
         Block2D:
           - Shape: triangle
             Color: [0.9, 0.1, 0.1]
             Scale: 0.5

     - Name: red_box
       MapCharacter: "2"
       InitialActions:
         - Action: box_counter
           ActionId: 1
       Observers:
         Sprite2D:
           - Image: gvgai/newset/blockR.png
         Block2D:
           - Shape: square
             Color: [0.5, 0.2, 0.2]
             Scale: 0.5
     - Name: red_block
       MapCharacter: b
       Observers:
         Sprite2D:
           - Image: gvgai/newset/blockR2.png
         Block2D:
           - Shape: square
             Color: [1.0, 0.0, 0.0]
             Scale: 1.0

     - Name: green_box
       MapCharacter: "3"
       InitialActions:
         - Action: box_counter
           ActionId: 1
       Observers:
         Sprite2D:
           - Image: gvgai/newset/blockG.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.5, 0.2]
             Scale: 0.5
     - Name: green_block
       MapCharacter: c
       Observers:
         Sprite2D:
           - Image: gvgai/newset/blockG2.png
         Block2D:
           - Shape: square
             Color: [0.0, 1.0, 0.0]
             Scale: 1.0

     - Name: blue_box
       MapCharacter: "1"
       InitialActions:
         - Action: box_counter
           ActionId: 1
       Observers:
         Sprite2D:
           - Image: gvgai/newset/blockB.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.2, 0.5]
             Scale: 0.5
     - Name: blue_block
       MapCharacter: a
       Observers:
         Sprite2D:
           - Image: gvgai/newset/blockB2.png
         Block2D:
           - Shape: square
             Color: [0.0, 0.0, 1.0]
             Scale: 1.0

     - Name: broken_box
       Observers:
         Sprite2D:
           - Image: gvgai/newset/block3.png
         Block2D:
           - Shape: triangle
             Color: [1.0, 0.0, 1.0]
             Scale: 1.0


