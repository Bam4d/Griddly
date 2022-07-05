.. _doc_butterflies_and_spiders:

Butterflies and Spiders
=======================

.. code-block::

   Single-Player/GVGAI/butterflies.yaml

Description
-------------

You want to catch all of the butterflies while also avoiding the spiders. Butterflies spawn slowly from cocoons.
The butterflies are also eaten by the spiders so you need to be fast to collect them.
You win the level as soon as there are no butterflies on the screen.


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
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-0.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-0.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-1.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-1.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-2.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-2.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-3.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-3.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 28x12
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-4.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-4.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-4.png
   * - .. list-table:: 

          * - Level ID
            - 5
          * - Size
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-5.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-5.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-5.png
   * - .. list-table:: 

          * - Level ID
            - 6
          * - Size
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-6.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-6.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-6.png
   * - .. list-table:: 

          * - Level ID
            - 7
          * - Size
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-7.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-7.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-7.png
   * - .. list-table:: 

          * - Level ID
            - 8
          * - Size
            - 28x11
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-8.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-8.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-8.png
   * - .. list-table:: 

          * - Level ID
            - 9
          * - Size
            - 28x12
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Block2D-9.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Vector-9.png
     - .. thumbnail:: img/Butterflies_and_Spiders-level-Sprite2D-9.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Butterflies-and-Spiders-v0')
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
     - wall
     - butterfly
     - cocoon
     - spider
     - catcher
   * - Map Char ->
     - `w`
     - `1`
     - `0`
     - `S`
     - `A`
   * - Block2D
     - .. image:: img/Butterflies_and_Spiders-tile-wall-Block2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-butterfly-Block2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-cocoon-Block2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-spider-Block2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-catcher-Block2D.png
   * - Vector
     - .. image:: img/Butterflies_and_Spiders-tile-wall-Vector.png
     - .. image:: img/Butterflies_and_Spiders-tile-butterfly-Vector.png
     - .. image:: img/Butterflies_and_Spiders-tile-cocoon-Vector.png
     - .. image:: img/Butterflies_and_Spiders-tile-spider-Vector.png
     - .. image:: img/Butterflies_and_Spiders-tile-catcher-Vector.png
   * - Sprite2D
     - .. image:: img/Butterflies_and_Spiders-tile-wall-Sprite2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-butterfly-Sprite2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-cocoon-Sprite2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-spider-Sprite2D.png
     - .. image:: img/Butterflies_and_Spiders-tile-catcher-Sprite2D.png


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


spider_random_movement
^^^^^^^^^^^^^^^^^^^^^^

:Relative: The actions are calculated relative to the object being controlled.

:Internal: This action can only be called from other actions, not by the player.

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


spawn_butterfly
^^^^^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

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


butterfly_random_movement
^^^^^^^^^^^^^^^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

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
     Name: Butterflies and Spiders
     Description: |
       You want to catch all of the butterflies while also avoiding the spiders. Butterflies spawn slowly from cocoons.
       The butterflies are also eaten by the spiders so you need to be fast to collect them.
       You win the level as soon as there are no butterflies on the screen.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/oryx/grass_15.png
       Block2D:
         TileSize: 24
     Player:
       AvatarObject: catcher
     Termination:
       Win:
         - eq: [butterfly:count, 0] # If there are no butterflies
       Lose:
         - eq: [catcher:count, 0] # If the catcher gets killed
     Levels:
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w . . 1 . . . . . 1 . . w . . . 0 . 0 . 0 . 0 w 0 0 0 w
         w . 1 . . . . . . . . . . . . . . . . . . . . w 0 0 0 w
         w . . . 1 . . . 0 . . . . . A . . . . . . . . w 0 0 0 w
         w w w w w w w w w w w w . . . . . . . . . . . . . 0 0 w
         w 0 . . . . . . . . . . . . . . . . . . w . . . . . w w
         w 0 . . . . . . 1 . . . . . . . . . . . . . . . . . . w
         w 0 . . . . . . . . . w w w w w . . . . 1 . . . . . 0 w
         w w w w w . . . . . . . . . . . . . . . . w . . . . . w
         w . . . . . . . . 0 . 0 . 0 . 0 . 0 . . . w 0 . . . 0 w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w . . w 0 w . . . . . . . . 0 . . . . . . . . w 0 w . w
         w . . . . . . . . . . . . . . . . . . . . . . . . . . w
         w . . . 1 . . . w . . . 1 . . . . . w w w . . . . . 1 w
         w . . . . . 1 . w . . . . 1 . 1 . . . 1 . . . . . . . w
         w 0 . . . . . . . w . . . . . . . . . . . . . . . . 0 w
         w . . . . . . . . . 1 . . . w w w w . . . 1 . . . . . w
         w . . . . 1 . . . . . . . . w . 1 . . . . . . 1 . . . w
         w . . . . . . . . . A . . . . . . . . . . . . . . . . w
         w . . w 0 w . . . . . . . . 0 . . . . . . . . w 0 w . w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w . . . . . . . . . . . . . . 1 . . . . . . . . . 0 . w
         w . . 0 0 0 0 . . . . . . . . 1 . . . . . . . . . . 0 w
         w . . . 0 0 . . . . . . 1 . . 1 . . w w w . . . . . . w
         w . . w . . . . . . 1 . . . . . . . . . . . . . . . . w
         w 0 0 w . . . 1 w w w w w w 1 w w . . . . . . A . . . w
         w . . w . . . . . . 1 . . . . . . . . . . . . . . . . w
         w . . . 0 0 . . . . . . 1 . . 1 . . w w w . . . . . . w
         w . . 0 0 0 0 . . . . . . . . 1 . . . . . . . . . . 0 w
         w . . . . . . . . . . . . . . 1 . . . . . . . . . 0 . w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w 0 0 w . . . . . . . . . . . . . . . . . . . . . . . w
         w 0 0 w . . . . . . . . . . . . . . . . . 1 . . . . . w
         w 0 0 w . . . . . . 1 . . . . . . . . . . . . . . . . w
         w . w w . . . . . . . . . . 1 . . . . 1 . . . 1 . . . w
         w . . . . . . 0 . . . . . . . . . . . . . . 1 . . . . w
         w . . . . . . . . . . . 1 . . . . . . . . . . 1 . . . w
         w . . . . . . . . . . . . 0 . . . . 1 . 1 . . . . . . w
         w . . . . . . . . . . . . . . . . . . . . . . w w w w w
         w . . . . . A . . . . . . . . . . . . . . . . . . 0 0 w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w . . . . . . . . . A . . . . . . . . . . . . . . . . w
         w . . . . . . . . . . . . . . . . . . . . . . . . . . w
         w . . . . . . . . . . . . . . . . . . . . . . . . . . w
         w . . . . . . . . . . . . . . . . . . . . . . . . . . w
         w w w w w w w w w w w w w . w w w w w w w w w w w w w w
         w . . . . . . . . . . . . . . . . . . . . . . . . . . w
         w . . . . . 1 . . . 1 . 1 . . 1 . . . . . . . w . . . w
         w . . . . . . . . . . . . . . . . . . . . . w . . 0 . w
         w . . . . 1 . . 1 . . 1 . . . . . . . . . w . 0 . . . w
         w . . . . . . . . . . . . . . . . . . . w . . 0 . . . w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w . . 1 . . . . . 1 . . w . . . 0 . 0 . 0 . 0 w 0 0 0 w
         w . 1 . S . . . . . . . . . . . . . . . . . . w 0 0 0 w
         w . . . 1 . . . 0 . . . . . A . . . . . . . . w 0 0 0 w
         w w w w w w w w w w w w . . . . . . S . S . . . . 0 0 w
         w 0 . . . . . . . . . . . . . . . . . . w . . . . . w w
         w 0 . . . . . . 1 . . . . . . . . . . . . . . . . . . w
         w 0 . . . . . . . . . w w w w w . . . . 1 . . . . . 0 w
         w w w w w . . . . . . . S S S S . . . . . w . . . . . w
         w . . . . . . . . 0 . 0 . 0 . 0 . 0 . . . w 0 . . . 0 w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w . . w 0 w . . . . . . . . 0 . . . . . . . . w 0 w . w
         w . . . . S . . . . . . . . . . . . . . . . . . . . . w
         w . . . 1 . . . w . . . 1 . . . S . w w w . . . . . 1 w
         w . . . . . 1 . w . . . . 1 . 1 . . . 1 . . . . . . . w
         w 0 . . . . . . . w S . . . . . S . . . . . . . . . 0 w
         w . . S S . . . . . 1 . . . w w w w . . . 1 . . . . . w
         w . . . . 1 . . . . . . . . w . 1 . . . . S . 1 . . . w
         w . . S . . . . . . A . . . . . . . . . . . . . . . . w
         w S S w 0 w . . . . . . . . 0 . . . S . . . . w 0 w . w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w . . . . . . . . . . . . . . 1 . . . . . . . . . 0 . w
         w . . 0 0 0 0 . . . . . . . . 1 . . . . . . . . . . 0 w
         w . . . 0 0 . . . . . . 1 . . 1 . . w w w . . . . . . w
         w . . w . . . . . . 1 . . . . . . . . . . . . . . . . w
         w 0 0 w . . . 1 w w w w w w 1 w w . . . . . . A . . . w
         w . . w . . . . . . 1 . . . . . . . . . . . . . . . . w
         w . . . 0 0 . . . . . . 1 . . 1 . . w w w . . . . . . w
         w . . 0 0 0 0 . . . . . . . . 1 . . . . . . . . . . 0 w
         w . . . . . . . . . . . . . . 1 . . . . . . . . . 0 . w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
         w 0 0 w . . . . . . . . . . . . . . . . . . . . . . . w
         w 0 0 w . . . . . . . . . . . . S . . . . 1 . . . . . w
         w 0 0 w . . . . . . 1 . . . . . S . . . . . . . . . . w
         w . w w . . . . . . . . . . 1 . S . . 1 . . . 1 . . . w
         w . . . . . . 0 . . . . . . . . S . . . . . 1 . . . . w
         w . . . . . . . . . . . 1 . . . S . . . . . . 1 . . . w
         w . . . . . . . . . . . . 0 . . S . 1 . 1 . . . . . . w
         w . . . . . . . . . . . . . . . S . . . . . . w w w w w
         w . . . . . A . . . . . . . . . . . . . . . . . . 0 0 w
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w 
         w . . . S . . . . . A . . . . . . . . . S . . . . . . w 
         w . . . . . . . . . . . . . . . S . . . . . . S . . . w 
         w . . . . . . S . . S . . . . . . . . . . . . . . . . w 
         w . . . . . . . . . . . . . . . S . . . . . S . . . . w 
         w w w w w w w w w w w w w . w w w w w w w w w w w w w w 
         w . . . . . . . . . . . . . . . . . . . . . . . . . . w 
         w . . . . . 1 . . . 1 . 1 . . 1 . . . . . . . w . . . w 
         w . . . . . . . . . . . . . . . . . . . . . w . . 0 . w 
         w . . . . 1 . . 1 . . 1 . . . . . . . . . w . 0 . . . w 
         w . . . . . . . . . . . . . . . . . . . w . . 0 . . . w 
         w w w w w w w w w w w w w w w w w w w w w w w w w w w w 

   Actions:
     - Name: spawn_butterfly
       InputMapping:
         Internal: true
       Behaviours:
         - Src:
             Object: cocoon
             Commands:
               - spawn: butterfly
               - exec:
                   Action: spawn_butterfly
                   Delay: 50
                   Randomize: true

           Dst:
             Object: _empty

         - Src:
             Object: cocoon
             Commands:
               - exec:
                   Action: spawn_butterfly
                   Delay: 50
                   Randomize: true
           Dst:
             Object: [cocoon, butterfly, catcher, wall]

     # Butterfly movement is different to spider movement
     - Name: butterfly_random_movement
       InputMapping:
         Internal: true
       Behaviours:

         # The butterfly moves into an empty space
         - Src:
             Object: butterfly
             Commands:
               - mov: _dest
               - exec:
                   Action: butterfly_random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: _empty
      
         # if the butterfly tries to move into anything but an empty spot
         - Src:
             Object: butterfly
             Commands:
               - exec:
                   Action: butterfly_random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: [ wall, spider, catcher, butterfly, cocoon ]

     # Define spider movement
     - Name: spider_random_movement
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
         Internal: true
       Behaviours:
         # Spider rotates on the spot
         - Src:
             Object: spider
             Commands:
               - rot: _dir
               - exec:
                   Action: spider_random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: spider

         # The catcher and the spider can move into empty space
         - Src:
             Object: spider
             Commands:
               - mov: _dest
               - exec:
                   Action: spider_random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: _empty

         # The spider will not move into the wall or the gem, but it needs to keep moving
         - Src:
             Object: spider
             Commands:
               - exec:
                   Action: spider_random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: wall

         # If the spider moves into a butterfly it dies
         - Src:
             Object: spider
             Commands:
               - mov: _dest
               - exec:
                   Action: spider_random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: butterfly
             Commands:
               - remove: true
               - reward: -1
         # if the spider moves into the catcher it dies
         - Src:
             Object: spider
           Dst:
             Object: catcher
             Commands:
               - remove: true
               - reward: -10

     # Define the move action
     - Name: move
       Behaviours:

         # If the catcher moves into a spider
         - Src:
             Object: catcher
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: spider

         # The catcher move into an empty space
         - Src:
             Object: catcher
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # If the catcher moves into a butterfly object, the butterfly is caught YAY!
         - Src:
             Object: catcher
             Commands:
               - mov: _dest
               - reward: 1
           Dst:
             Object: butterfly
             Commands:
               - remove: true

   Objects:
     - Name: wall
       MapCharacter: 'w'
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
             Image:
               - oryx/oryx_fantasy/wall9-0.png
               - oryx/oryx_fantasy/wall9-1.png
               - oryx/oryx_fantasy/wall9-2.png
               - oryx/oryx_fantasy/wall9-3.png
               - oryx/oryx_fantasy/wall9-4.png
               - oryx/oryx_fantasy/wall9-5.png
               - oryx/oryx_fantasy/wall9-6.png
               - oryx/oryx_fantasy/wall9-7.png
               - oryx/oryx_fantasy/wall9-8.png
               - oryx/oryx_fantasy/wall9-9.png
               - oryx/oryx_fantasy/wall9-10.png
               - oryx/oryx_fantasy/wall9-11.png
               - oryx/oryx_fantasy/wall9-12.png
               - oryx/oryx_fantasy/wall9-13.png
               - oryx/oryx_fantasy/wall9-14.png
               - oryx/oryx_fantasy/wall9-15.png
         Block2D:
           - Shape: square
             Color: [0.7, 0.7, 0.7]
             Scale: 0.9

     - Name: butterfly
       InitialActions:
         - Action: butterfly_random_movement
           Delay: 3
           Randomize: true
       MapCharacter: '1'
       Observers:
         Sprite2D:
           - Image: gvgai/newset/butterfly1.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 0.0, 1.0]
             Scale: 0.3

     - Name: cocoon
       MapCharacter: '0'
       InitialActions:
         - Action: spawn_butterfly
           Delay: 50
           Randomize: true
       Observers:
         Sprite2D:
           - Image: gvgai/newset/cocoonb1.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 0.5

     - Name: spider
       InitialActions:
         - Action: spider_random_movement
           Delay: 3
           Randomize: true
       MapCharacter: 'S'
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/spider1.png
         Block2D:
           - Shape: triangle
             Color: [1.0, 0.0, 0.0]
             Scale: 0.5

     - Name: catcher
       MapCharacter: 'A'
       Observers:
         Sprite2D:
           - Image: gvgai/newset/girl5.png
         Block2D:
           - Shape: triangle
             Color: [1.0, 1.0, 1.0]
             Scale: 0.8



