.. _doc_spiders:

Spiders
=======

.. code-block::

   Single-Player/Mini-Grid/minigrid-spiders.yaml

Description
-------------

A port of the games provided in the https://github.com/maximecb/gym-minigrid Dynamic obstacles environment, but you're a gnome avoiding ghosts to get to a gem.

Levels
---------

.. list-table:: Levels
   :class: level-gallery
   :header-rows: 1

   * - 
     - Block2D
     - Vector
     - Sprite2D
     - Isometric
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 6x6
     - .. thumbnail:: img/Spiders-level-Block2D-0.png
     - .. thumbnail:: img/Spiders-level-Vector-0.png
     - .. thumbnail:: img/Spiders-level-Sprite2D-0.png
     - .. thumbnail:: img/Spiders-level-Isometric-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 7x7
     - .. thumbnail:: img/Spiders-level-Block2D-1.png
     - .. thumbnail:: img/Spiders-level-Vector-1.png
     - .. thumbnail:: img/Spiders-level-Sprite2D-1.png
     - .. thumbnail:: img/Spiders-level-Isometric-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 8x8
     - .. thumbnail:: img/Spiders-level-Block2D-2.png
     - .. thumbnail:: img/Spiders-level-Vector-2.png
     - .. thumbnail:: img/Spiders-level-Sprite2D-2.png
     - .. thumbnail:: img/Spiders-level-Isometric-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 10x10
     - .. thumbnail:: img/Spiders-level-Block2D-3.png
     - .. thumbnail:: img/Spiders-level-Vector-3.png
     - .. thumbnail:: img/Spiders-level-Sprite2D-3.png
     - .. thumbnail:: img/Spiders-level-Isometric-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 19x18
     - .. thumbnail:: img/Spiders-level-Block2D-4.png
     - .. thumbnail:: img/Spiders-level-Vector-4.png
     - .. thumbnail:: img/Spiders-level-Sprite2D-4.png
     - .. thumbnail:: img/Spiders-level-Isometric-4.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Spiders-v0')
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
     - spider
     - gem
     - gnome
   * - Map Char ->
     - `W`
     - `G`
     - `g`
     - `A`
   * - Block2D
     - .. image:: img/Spiders-tile-wall-Block2D.png
     - .. image:: img/Spiders-tile-spider-Block2D.png
     - .. image:: img/Spiders-tile-gem-Block2D.png
     - .. image:: img/Spiders-tile-gnome-Block2D.png
   * - Vector
     - .. image:: img/Spiders-tile-wall-Vector.png
     - .. image:: img/Spiders-tile-spider-Vector.png
     - .. image:: img/Spiders-tile-gem-Vector.png
     - .. image:: img/Spiders-tile-gnome-Vector.png
   * - Sprite2D
     - .. image:: img/Spiders-tile-wall-Sprite2D.png
     - .. image:: img/Spiders-tile-spider-Sprite2D.png
     - .. image:: img/Spiders-tile-gem-Sprite2D.png
     - .. image:: img/Spiders-tile-gnome-Sprite2D.png
   * - Isometric
     - .. image:: img/Spiders-tile-wall-Isometric.png
     - .. image:: img/Spiders-tile-spider-Isometric.png
     - .. image:: img/Spiders-tile-gem-Isometric.png
     - .. image:: img/Spiders-tile-gnome-Isometric.png


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
   * - 4
     - Move Backwards


random_movement
^^^^^^^^^^^^^^^

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


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Spiders
     Description: A port of the games provided in the https://github.com/maximecb/gym-minigrid Dynamic obstacles environment, but you're a gnome avoiding ghosts to get to a gem.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: oryx/oryx_fantasy/floor2-2.png
       Isometric:
         TileSize: [32, 48]
         IsoTileHeight: 16
         IsoTileDepth: 4
         BackgroundTile: oryx/oryx_iso_dungeon/grass-1.png
       Block2D:
         TileSize: 24
     Player:
       Observer:
         RotateWithAvatar: true
         TrackAvatar: true
         Height: 7
         Width: 7
         OffsetX: 0
         OffsetY: 3
       AvatarObject: gnome
     Termination:
       Win:
         - eq: [gem:count, 0] # If there are no boxes left
       Lose:
         - eq: [gnome:count, 0] # If there are no boxes left
     Levels:
       - |
         W  W  W  W  W  W
         W  A  .  .  .  W
         W  .  .  .  .  W
         W  .  G  .  .  W
         W  .  .  .  g  W
         W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W
         W  A  .  .  .  .  W
         W  .  .  .  .  .  W
         W  .  .  G  .  .  W
         W  .  .  .  .  .  W
         W  .  G  .  .  g  W
         W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  W
         W  .  .  .  .  G  .  W
         W  .  .  .  .  .  .  W
         W  .  G  .  G  .  .  W
         W  .  .  .  .  .  .  W
         W  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  .  .  W
         W  .  .  G  .  .  .  .  .  W
         W  .  .  .  .  .  .  G  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  G  .  .  .  W
         W  .  G  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  G  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  G  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  G  .  .  .  .  .  G  .  .  .  .  .  G  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  G  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  G  .  .  G  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W

   Actions:
     # Define action that cannot be controlled by the player. (In this case the spider movement)
     - Name: random_movement
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
                   Action: random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: spider

         # The gnome and the spider can move into empty space
         - Src:
             Object: spider
             Commands:
               - mov: _dest
               - exec:
                   Action: random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: _empty

         # The spider will not move into the wall or the gem, but it needs to keep moving
         - Src:
             Object: spider
             Commands:
               - exec:
                   Action: random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: [wall, gem]

         # If the gnome moves into a spider
         - Src:
             Object: spider
           Dst:
             Object: gnome
             Commands:
               - remove: true
               - reward: -1

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
           4:
             Description: Move Backwards
             VectorToDest: [0, 1]
             OrientationVector: [0, -1]
         Relative: true
       Behaviours:
         # Tell the gnome to rotate if it performs an action on itself (Rotate left and Rotate right actions)
         - Src:
             Object: gnome
             Commands:
               - rot: _dir
           Dst:
             Object: gnome

         # If the gnome moves into a spider
         - Src:
             Object: gnome
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: spider

         # The gnome and the spider can move into empty space
         - Src:
             Object: gnome
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # If the gnome moves into a gem object, the stick is removed, triggering a win condition
         - Src:
             Object: gnome
             Commands:
               - reward: 1
           Dst:
             Object: gem
             Commands:
               - remove: true

   Objects:
     - Name: wall
       MapCharacter: W
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
             Image:
               - oryx/oryx_fantasy/wall2-0.png
               - oryx/oryx_fantasy/wall2-1.png
               - oryx/oryx_fantasy/wall2-2.png
               - oryx/oryx_fantasy/wall2-3.png
               - oryx/oryx_fantasy/wall2-4.png
               - oryx/oryx_fantasy/wall2-5.png
               - oryx/oryx_fantasy/wall2-6.png
               - oryx/oryx_fantasy/wall2-7.png
               - oryx/oryx_fantasy/wall2-8.png
               - oryx/oryx_fantasy/wall2-9.png
               - oryx/oryx_fantasy/wall2-10.png
               - oryx/oryx_fantasy/wall2-11.png
               - oryx/oryx_fantasy/wall2-12.png
               - oryx/oryx_fantasy/wall2-13.png
               - oryx/oryx_fantasy/wall2-14.png
               - oryx/oryx_fantasy/wall2-15.png
         Block2D:
           - Shape: square
             Color: [0.7, 0.7, 0.7]
             Scale: 1.0
         Isometric:
           - Image: oryx/oryx_iso_dungeon/bush-1.png

     - Name: spider
       InitialActions:
         - Action: random_movement
           Delay: 3
           Randomize: true
       MapCharacter: G
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/spider1.png
         Block2D:
           - Shape: triangle
             Color: [1.0, 0.0, 0.0]
             Scale: 0.8
         Isometric:
           - Image: oryx/oryx_iso_dungeon/avatars/spider-1.png

     - Name: gem
       MapCharacter: g
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/ore-6.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 0.5
         Isometric:
           - Image: oryx/oryx_iso_dungeon/ore-6.png

     - Name: gnome
       MapCharacter: A
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/gnome1.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 0.0, 1.0]
             Scale: 0.8
         Isometric:
           - Image: oryx/oryx_iso_dungeon/avatars/gnome-1.png

