.. _doc_robot_tag_12v12:

Robot Tag 12v12
===============

.. code-block::

   Multi-Agent/robot_tag_12.yaml

Description
-------------

Robots start randomly as "tagged" or not, robots can "tag" other robots. Any robot that is "tagged" 3 times dies.

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
            - 9x10
     - .. thumbnail:: img/Robot_Tag_12v12-level-Block2D-0.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Vector-0.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Sprite2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 22x22
     - .. thumbnail:: img/Robot_Tag_12v12-level-Block2D-1.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Vector-1.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Sprite2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 22x22
     - .. thumbnail:: img/Robot_Tag_12v12-level-Block2D-2.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Vector-2.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Sprite2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 40x46
     - .. thumbnail:: img/Robot_Tag_12v12-level-Block2D-3.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Vector-3.png
     - .. thumbnail:: img/Robot_Tag_12v12-level-Sprite2D-3.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Robot-Tag-12v12-v0')
       env.reset()
    
       # Replace with your own control algorithm!
       for s in range(1000):
           obs, reward, done, info = env.step(env.action_space.sample())
           for p in range(env.player_count):
               env.render(observer=p) # Renders the environment from the perspective of a single player

           env.render(observer='global') # Renders the entire environment
        
           if done:
               env.reset()


Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - tagger
     - moveable_wall
     - fixed_wall
   * - Map Char ->
     - `f`
     - `m`
     - `W`
   * - Block2D
     - .. image:: img/Robot_Tag_12v12-tile-tagger-Block2D.png
     - .. image:: img/Robot_Tag_12v12-tile-moveable_wall-Block2D.png
     - .. image:: img/Robot_Tag_12v12-tile-fixed_wall-Block2D.png
   * - Vector
     - .. image:: img/Robot_Tag_12v12-tile-tagger-Vector.png
     - .. image:: img/Robot_Tag_12v12-tile-moveable_wall-Vector.png
     - .. image:: img/Robot_Tag_12v12-tile-fixed_wall-Vector.png
   * - Sprite2D
     - .. image:: img/Robot_Tag_12v12-tile-tagger-Sprite2D.png
     - .. image:: img/Robot_Tag_12v12-tile-moveable_wall-Sprite2D.png
     - .. image:: img/Robot_Tag_12v12-tile-fixed_wall-Sprite2D.png


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


initialize_is_tagged
^^^^^^^^^^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - Initialize Tagged
   * - 2
     - Initialize Not Tagged


tag
^^^

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
     Name: Robot Tag 12v12
     Description: Robots start randomly as "tagged" or not, robots can "tag" other robots. Any robot that is "tagged" 3 times dies.
     Observers:
       Block2D:
         TileSize: 24
       Sprite2D:
         TileSize: 24
         BackgroundTile: oryx/oryx_fantasy/floor1-1.png
       Vector:
         IncludePlayerId: true
         IncludeVariables: true
     Variables:
       - Name: player_done
         InitialValue: 0
         PerPlayer: true
       - Name: tagged_count
         InitialValue: 0
     Player:
       Count: 12
       Observer:
         RotateWithAvatar: true
         TrackAvatar: true
         Height: 9
         Width: 9
         OffsetX: 0
         OffsetY: 0
       AvatarObject: tagger
     Termination:
       End:
         - eq: [ tagged_count, 0 ]

     Levels:
       - |
         W   W   W   W   W   W   W   W   W
         W   .   .   f2  .   f12 .   .   W
         W   .   .   .   .   .   .   .   W
         W   f1  .   f3  .   f10 .   f11 W
         W   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   W
         W   f4  .   f5  .   f7  .   f8  W
         W   .   .   .   .   .   .   .   W
         W   .   .   f6  .   f9  .   .   W
         W   W   W   W   W   W   W   W   W
       - |
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   f2  .   .   .   .   .   .   .   .   .   .   .   .   .   .   f12 .   .   W
         W   .   f1  f3  .   .   .   .   .   .   .   .   .   .   .   .   .   .   f10 f11 .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   W   W   W   W   W   W   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   W   W   W   W   W   W   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   f4  f5  .   .   .   .   .   .   .   .   .   .   .   .   .   .   f7  f8  .   W
         W   .   .   f6  .   .   .   .   .   .   .   .   .   .   .   .   .   .   f9  .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
       - |
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
         W   .   .   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   .   .   W
         W   .   .   f2  .   m   .   .   .   .   .   .   .   .   .   .   m   .   f12 .   .   W
         W   .   f1  f3  .   m   .   .   .   .   .   .   .   .   .   .   m   .   f10 f11 .   W
         W   .   .   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   .   .   W
         W   .   .   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   .   .   W
         W   m   m   m   m   m   .   .   W   W   W   W   W   W   .   .   m   m   m   m   m   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   W   .   .   .   .   .   .   W   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   .   W
         W   m   m   m   m   m   .   .   W   W   W   W   W   W   .   .   m   m   m   m   m   W
         W   .   .   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   .   .   W
         W   .   .   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   .   .   W
         W   .   f4  f5  .   m   .   .   .   .   .   .   .   .   .   .   m   .   f7  f8  .   W
         W   .   .   f6  .   m   .   .   .   .   .   .   .   .   .   .   m   .   f9  .   .   W
         W   .   .   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   .   .   W
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
       - |
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
         W   .   .   .   .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   .   .   .   W
         W   .   .   .   .   m   f2  .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   f12 m   .   .   .   .   W
         W   .   .   f3  .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   f10 .   .   W
         W   .   .   .   .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   .   .   .   W
         W   .   .   .   .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   .   .   .   W
         W   m   m   m   m   m   .   .   .   .   .   m   .   .   m   .   .   W   W   W   W   W   W   .   .   m   .   .   m   .   .   .   .   .   m   m   m   m   m   W
         W   .   f1  .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   f11 .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   W   .   .   .   .   .   .   W   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   W   .   .   .   .   .   .   W   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   W   .   .   .   .   .   .   W   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   W   .   .   .   .   .   .   W   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   W
         W   .   f4  .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   f8  .   W
         W   m   m   m   m   m   .   .   .   .   .   m   .   .   m   .   .   W   W   W   W   W   W   .   .   m   .   .   m   .   .   .   .   .   m   m   m   m   m   W
         W   .   .   .   .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   .   .   .   W
         W   .   .   .   .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   .   .   .   W
         W   .   .   f5  .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   f7  .   .   W
         W   .   .   .   .   m   f6  .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   f9  m   .   .   .   .   W
         W   .   .   .   .   m   .   .   .   .   .   m   .   .   m   .   .   .   .   .   .   .   .   .   .   m   .   .   m   .   .   .   .   .   m   .   .   .   .   W
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W


   Actions:

     # Taggers have a random chance of starting in a tagged state
     - Name: initialize_is_tagged
       InputMapping:
         Internal: true
         Inputs:
           1:
             Description: Initialize Tagged
           2:
             Description: Initialize Not Tagged
             VectorToDest: [ -1, 0 ]

       Behaviours:
         - Src:
             Object: tagger
             Preconditions:
               - eq: [ src._playerId, dst._playerId ]
             Commands:
               - set_tile: 1
               - set: [ is_tagged, 1 ]
               - incr: tagged_count
           Dst:
             Object: tagger

     - Name: tag
       Behaviours:
         - Src:
             Object: tagger
             Preconditions:
               - eq: [ src.is_tagged, 1 ]
               - eq: [ dst.is_tagged, 0 ]
             Commands:
               - reward: 2
               - set_tile: 0
               - set: [ is_tagged, 0 ]
           Dst:
             Object: tagger
             Commands:
               - set_tile: 1
               - set: [ is_tagged, 1 ]
               - reward: -2
               - incr: times_tagged
               - eq:
                   Arguments: [ times_tagged, 3 ]
                   Commands:
                     - set: [ player_done, 1 ]
                     - decr: tagged_count
                     - reward: -5
                     - remove: true

     - Name: move
       Behaviours:
         - Src:
             Object: [ tagger, moveable_wall ]
             Commands:
               - mov: _dest # mov will move the object, _dest is the destination location of the action
           Dst:
             Object: _empty

         - Src:
             Object: tagger
             Commands:
               - mov: _dest
           Dst:
             Object: moveable_wall
             Commands:
               - cascade: _dest

   Objects:
     - Name: tagger
       MapCharacter: f
       InitialActions:
         - Action: initialize_is_tagged
           Randomize: true
       Variables:
         - Name: is_tagged
           InitialValue: 0
         - Name: times_tagged
           InitialValue: 0
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/robot1.png
           - Image: oryx/oryx_fantasy/avatars/fireguy1.png
         Block2D:
           - Shape: triangle
             Color: [ 0.2, 0.2, 0.9 ]
             Scale: 0.5
           - Shape: triangle
             Color: [ 0.9, 0.2, 0.2 ]
             Scale: 1.0

     - Name: moveable_wall
       MapCharacter: m
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/wall4-0.png
         Block2D:
           - Color: [ 0.8, 0.8, 0.8 ]
             Shape: square

     - Name: fixed_wall
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
           - Color: [ 0.5, 0.5, 0.5 ]
             Shape: square


