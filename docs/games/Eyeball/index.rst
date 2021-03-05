.. _doc_eyeball

Eyeball
=======

.. code-block::

   Single-Player/Mini-Grid/minigrid-eyeball.yaml

Description
-------------

A port of the games provided in the https://github.com/maximecb/gym-minigrid 4 Rooms environment, but you're a giant eye looking for it's eyedrops because everything is yellow and it hurts to look at.

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
            - 19x19
     - .. thumbnail:: img/Eyeball-level-Vector-0.png
     - .. thumbnail:: img/Eyeball-level-Sprite2D-0.png
     - .. thumbnail:: img/Eyeball-level-Block2D-0.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Eyeball-v0')
       env.reset()
    
       # Replace with your own control algorithm!
       for s in range(1000):
           obs, reward, done, info = env.step(env.action_space.sample())
           env.render() # Renders the environment from the perspective of a single player

           env.render(observer='global') # Renders the entire environment
        
           if done:
               emv.reset()


Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - wall
     - eye_drops
     - eyeball
   * - Map Char ->
     - `W`
     - `g`
     - `A`
   * - Vector
     - .. image:: img/Eyeball-tile-wall-Vector.png
     - .. image:: img/Eyeball-tile-eye_drops-Vector.png
     - .. image:: img/Eyeball-tile-eyeball-Vector.png
   * - Sprite2D
     - .. image:: img/Eyeball-tile-wall-Sprite2D.png
     - .. image:: img/Eyeball-tile-eye_drops-Sprite2D.png
     - .. image:: img/Eyeball-tile-eyeball-Sprite2D.png
   * - Block2D
     - .. image:: img/Eyeball-tile-wall-Block2D.png
     - .. image:: img/Eyeball-tile-eye_drops-Block2D.png
     - .. image:: img/Eyeball-tile-eyeball-Block2D.png


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
     Name: Eyeball
     Description: A port of the games provided in the https://github.com/maximecb/gym-minigrid 4 Rooms environment, but you're a giant eye looking for it's eyedrops because everything is yellow and it hurts to look at.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: oryx/oryx_fantasy/floor7-1.png
     Player:
       Observer:
         RotateWithAvatar: true
         TrackAvatar: true
         Height: 7
         Width: 7
         OffsetX: 0
         OffsetY: 3
       AvatarObject: eyeball
     Termination:
       Win:
         - eq: [eye_drops:count, 0] # If there are no boxes left
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
         # Tell the agent to rotate if the eyeball performs an action on itself
         - Src:
             Object: eyeball
             Commands:
               - rot: _dir
           Dst:
             Object: eyeball

         # The agent can move around freely in empty and always rotates the direction it is travelling
         - Src:
             Object: eyeball
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # If the eyeball moves into a eye_drops object, the eye_drops is removed, triggering a win condition
         - Src:
             Object: eyeball
             Commands:
               - reward: 1
           Dst:
             Object: eye_drops
             Commands:
               - remove: true

   Objects:
     - Name: wall
       MapCharacter: W
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
             Image:
               - oryx/oryx_fantasy/wall7-0.png
               - oryx/oryx_fantasy/wall7-1.png
               - oryx/oryx_fantasy/wall7-2.png
               - oryx/oryx_fantasy/wall7-3.png
               - oryx/oryx_fantasy/wall7-4.png
               - oryx/oryx_fantasy/wall7-5.png
               - oryx/oryx_fantasy/wall7-6.png
               - oryx/oryx_fantasy/wall7-7.png
               - oryx/oryx_fantasy/wall7-8.png
               - oryx/oryx_fantasy/wall7-9.png
               - oryx/oryx_fantasy/wall7-10.png
               - oryx/oryx_fantasy/wall7-11.png
               - oryx/oryx_fantasy/wall7-12.png
               - oryx/oryx_fantasy/wall7-13.png
               - oryx/oryx_fantasy/wall7-14.png
               - oryx/oryx_fantasy/wall7-15.png
         Block2D:
           - Shape: square
             Color: [0.7, 0.7, 0.7]
             Scale: 1.0

     - Name: eye_drops
       MapCharacter: g
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/potion-1.png
         Block2D:
           - Shape: square
             Color: [0.0, 1.0, 0.0]
             Scale: 0.8

     - Name: eyeball
       MapCharacter: A
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/eye1.png
         Block2D:
           - Shape: triangle
             Color: [1.0, 0.0, 0.0]
             Scale: 1.0


