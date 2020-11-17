Doggo
=====

Description
-------------

A port of the games provided in the https://github.com/maximecb/gym-minigrid Empty environment, but you're a doggo fetching a stick.

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
            - 6x6
     - .. thumbnail:: img/Doggo-level-Vector-0.png
     - .. thumbnail:: img/Doggo-level-Sprite2D-0.png
     - .. thumbnail:: img/Doggo-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 7x7
     - .. thumbnail:: img/Doggo-level-Vector-1.png
     - .. thumbnail:: img/Doggo-level-Sprite2D-1.png
     - .. thumbnail:: img/Doggo-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 8x8
     - .. thumbnail:: img/Doggo-level-Vector-2.png
     - .. thumbnail:: img/Doggo-level-Sprite2D-2.png
     - .. thumbnail:: img/Doggo-level-Block2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 10x10
     - .. thumbnail:: img/Doggo-level-Vector-3.png
     - .. thumbnail:: img/Doggo-level-Sprite2D-3.png
     - .. thumbnail:: img/Doggo-level-Block2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 19x18
     - .. thumbnail:: img/Doggo-level-Vector-4.png
     - .. thumbnail:: img/Doggo-level-Sprite2D-4.png
     - .. thumbnail:: img/Doggo-level-Block2D-4.png

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

       env = gym.make('GDY-Doggo-v0')
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
           'Doggo-Adv',
           'Single-Player/Mini-Grid/minigrid-doggo.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
       )

       env = gym.make('GDY-Doggo-Adv-v0')
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
     - wall
     - stick
     - doggo
   * - Map Char ->
     - W
     - g
     - A
   * - Vector
     - .. image:: img/Doggo-tile-wall-Vector.png
     - .. image:: img/Doggo-tile-stick-Vector.png
     - .. image:: img/Doggo-tile-doggo-Vector.png
   * - Sprite2D
     - .. image:: img/Doggo-tile-wall-Sprite2D.png
     - .. image:: img/Doggo-tile-stick-Sprite2D.png
     - .. image:: img/Doggo-tile-doggo-Sprite2D.png
   * - Block2D
     - .. image:: img/Doggo-tile-wall-Block2D.png
     - .. image:: img/Doggo-tile-stick-Block2D.png
     - .. image:: img/Doggo-tile-doggo-Block2D.png


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
     Name: Doggo
     Description: A port of the games provided in the https://github.com/maximecb/gym-minigrid Empty environment, but you're a doggo fetching a stick.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: oryx/oryx_fantasy/floor9-3.png
     Player:
       Observer:
         RotateWithAvatar: true
         TrackAvatar: true
         Height: 7
         Width: 7
         OffsetX: 0
         OffsetY: 3
       AvatarObject: doggo
     Termination:
       Win:
         - eq: [stick:count, 0] # If there are no boxes left
     Levels:
       - |
         W  W  W  W  W  W
         W  A  .  .  .  W
         W  .  .  .  .  W
         W  .  .  .  .  W
         W  .  .  .  g  W
         W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W
         W  A  .  .  .  .  W
         W  .  .  .  .  .  W
         W  .  .  .  .  .  W
         W  .  .  .  .  .  W
         W  .  .  .  .  g  W
         W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  W
         W  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  W
         W  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W

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
         # Tell the agent to rotate if the doggo performs an action on itself
         - Src:
             Object: doggo
             Commands:
               - rot: _dir
           Dst:
             Object: doggo

         # The agent can move around freely in empty and always rotates the direction it is travelling
         - Src:
             Object: doggo
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # If the doggo moves into a stick object, the stick is removed, triggering a win condition
         - Src:
             Object: doggo
             Commands:
               - reward: 1
           Dst:
             Object: stick
             Commands:
               - remove: true

   Objects:
     - Name: wall
       MapCharacter: W
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
             Scale: 1.0

     - Name: stick
       MapCharacter: g
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/stick-0.png
         Block2D:
           - Shape: square
             Color: [0.0, 1.0, 0.0]
             Scale: 0.8

     - Name: doggo
       MapCharacter: A
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/doggo1.png
         Block2D:
           - Shape: triangle
             Color: [1.0, 0.0, 0.0]
             Scale: 1.0


