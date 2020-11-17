Partially Observable Labyrinth
==============================

Description
-------------

Its a maze, find your way out. Watch out for spikey things. In this version the observation space for the player is partial.

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
            - 16x14
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Vector-0.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Sprite2D-0.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 16x14
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Vector-1.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Sprite2D-1.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 16x14
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Vector-2.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Sprite2D-2.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Block2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 16x14
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Vector-3.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Sprite2D-3.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Block2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 16x14
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Vector-4.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Sprite2D-4.png
     - .. thumbnail:: img/Partially_Observable_Labyrinth-level-Block2D-4.png

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

       env = gym.make('GDY-Partially-Observable-Labyrinth-v0')
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
           'Partially-Observable-Labyrinth-Adv',
           'Single-Player/GVGAI/labyrinth_partially_observable.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
       )

       env = gym.make('GDY-Partially-Observable-Labyrinth-Adv-v0')
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
     - exit
     - trap
     - wall
   * - Map Char ->
     - A
     - x
     - t
     - w
   * - Vector
     - .. image:: img/Partially_Observable_Labyrinth-tile-avatar-Vector.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-exit-Vector.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-trap-Vector.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-wall-Vector.png
   * - Sprite2D
     - .. image:: img/Partially_Observable_Labyrinth-tile-avatar-Sprite2D.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-exit-Sprite2D.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-trap-Sprite2D.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-wall-Sprite2D.png
   * - Block2D
     - .. image:: img/Partially_Observable_Labyrinth-tile-avatar-Block2D.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-exit-Block2D.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-trap-Block2D.png
     - .. image:: img/Partially_Observable_Labyrinth-tile-wall-Block2D.png


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
     Name: Partially Observable Labyrinth
     Description: Its a maze, find your way out. Watch out for spikey things. In this version the observation space for the player is partial.
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
         - eq: [exit:count, 0]
       Lose:
         - eq: [avatar:count, 0]
     Levels:
       - |
         w w w w w w w w w w w w w w w w
         w w . . . . . w w w . . . . x w
         w w . w w w . w w w . w w w w w
         w w . w . w . . . . . . . w t w
         w w . w . w w w w . w w w w . w
         w . . . . . . w w w w . . . . w
         w . w w w w . w w w w . w w w w
         w . . . . w . . . . . . . . . w
         w w w w w w . w w w w . w w . w
         w . . . . . . . . . . . . . . w
         w . w w w w . w w w . w w w . w
         w . w . w w . w w w . w w w w w
         w A w . . . . . t . . . . . . w
         w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w
         w w w w w w w w w w w w w . . w
         w x t t t t w . . . . . w w . w
         w . . . . . . . w w w . . . . w
         w w w w w w w w w . w w w w . w
         w . . . w w w w w . w w w w . w
         w w w . . . . . . . . . w w . w
         w t w . w w w w w . w . . . . w
         w . w w w w w w w . w w w w . w
         w . w w w . . . . . w w w w . w
         w . w w w . w w w w w w w w . w
         w . w w w . w w w . w w w w w w
         w . . . . . . . . . . . . . A w
         w w w w w w w w w w w w w w w w
       - | 
         w w w w w w w w w w w w w w w w
         w w w w w w w . x w w . . . t w
         w . . . . . . . w . w . w w w w
         w . w w . w w w w . w . . . . w
         w . w w . w w w . . w w w w . w
         w w w t . w w w . w w w . . . w
         w . . . . w w w . w w w . w w w
         w . w w w w w w . w w w . w w w
         w . t . . . . . . . . . . . . w
         w . w . w w w w . w w w . w w w
         w . w . . . . w . w w w . w w w
         w . w w w w . w . w w w . w w w
         w . . . . . . w A w w w . w w w
         w w w w w w w w w w w w w w w w
       - |
         w w w w w w w w w w w w w w w w
         w . . . . . . . . . . . . . . w
         w . w w w w w w w w w w w w . w
         w . w . . . . . . . . . . w . w
         w . w w w . w w w . w w . w . w
         w . w w w . w w w t w w . w . w
         w . w w w . w w w . w w . w . w
         w A . . . . . . . . . . . t x w
         w . w w w . w . w w w w . w w w
         w . w w w w w . w w w w . w w w
         w . . . . . . . w w w w . w w w
         w . w w w w w . w w w w . w w w
         w . . . . . . . . . . . . . t w
         w w w w w w w w w w w w w w w w
       - | 
         w w w w w w w w w w w w w w w w
         w . . . . . . . . . . . . t A w
         w . w w w w w w w w w w . w . w
         w . . . . . . . . . . w . w . w
         w w w w w w w w w t . w . w . w
         w . . . . . . . . . . w . w . w
         w . w w w w w w w w w w . w . w
         w . w . . . w . . . w w . w . w
         w . w . w . . . w . . w . w . w
         w . w . w w w w w w w w . w . w
         w . w . . . . . . . . . . w . w
         w . w . t w w w w w w w . w . w
         w x w . . . . . . . . w . . . w
         w w w w w w w w w w w w w w w w

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

         # Avatar can move into empty space
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # If Avatar hits a trap, remove it
         - Src:
             Object: avatar
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: trap

         # If Avatar hits the exit, remove the exit
         - Src:
             Object: avatar
             Commands:
               - reward: 1
           Dst:
             Object: exit
             Commands:
               - remove: true            

   Objects:
     - Name: avatar
       MapCharacter: A
       Observers:
         Sprite2D:
           - Image: gvgai/newset/girl1.png
         Block2D:
           - Shape: triangle
             Color: [0.2, 0.8, 0.2]
             Scale: 0.6

     - Name: exit
       MapCharacter: x
       Observers:
         Sprite2D:
           - Image: gvgai/newset/exit2.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.2, 0.8]
             Scale: 0.7

     - Name: trap
       MapCharacter: t
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/spike2.png
         Block2D:
           - Shape: triangle
             Color: [1.0, 0.0, 0.0]
             Scale: 0.3

     - Name: wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
             Image:
               - oryx/oryx_fantasy/wall8-0.png
               - oryx/oryx_fantasy/wall8-1.png
               - oryx/oryx_fantasy/wall8-2.png
               - oryx/oryx_fantasy/wall8-3.png
               - oryx/oryx_fantasy/wall8-4.png
               - oryx/oryx_fantasy/wall8-5.png
               - oryx/oryx_fantasy/wall8-6.png
               - oryx/oryx_fantasy/wall8-7.png
               - oryx/oryx_fantasy/wall8-8.png
               - oryx/oryx_fantasy/wall8-9.png
               - oryx/oryx_fantasy/wall8-10.png
               - oryx/oryx_fantasy/wall8-11.png
               - oryx/oryx_fantasy/wall8-12.png
               - oryx/oryx_fantasy/wall8-13.png
               - oryx/oryx_fantasy/wall8-14.png
               - oryx/oryx_fantasy/wall8-15.png
         Block2D:
           - Shape: square
             Color: [0.5, 0.5, 0.5]
             Scale: 0.9


