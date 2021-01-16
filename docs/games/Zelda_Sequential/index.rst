Zelda Sequential
================

Description
-------------

A port of the GVGAI game "Zelda". Pick up keys to reach doors in the correct order and avoid enemies. For example, previously you could go -- key -> door -> door. But now you would need to go -- key -> door --> key --> door.

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
            - 13x9
     - .. thumbnail:: img/Zelda_Sequential-level-Vector-0.png
     - .. thumbnail:: img/Zelda_Sequential-level-Sprite2D-0.png
     - .. thumbnail:: img/Zelda_Sequential-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 13x9
     - .. thumbnail:: img/Zelda_Sequential-level-Vector-1.png
     - .. thumbnail:: img/Zelda_Sequential-level-Sprite2D-1.png
     - .. thumbnail:: img/Zelda_Sequential-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 30x9
     - .. thumbnail:: img/Zelda_Sequential-level-Vector-2.png
     - .. thumbnail:: img/Zelda_Sequential-level-Sprite2D-2.png
     - .. thumbnail:: img/Zelda_Sequential-level-Block2D-2.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Zelda-Sequential-v0')
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
     - attack_fire
     - key
     - goal
     - spider
     - wall
   * - Map Char ->
     - `A`
     - `x`
     - `+`
     - `g`
     - `3`
     - `w`
   * - Vector
     - .. image:: img/Zelda_Sequential-tile-avatar-Vector.png
     - .. image:: img/Zelda_Sequential-tile-attack_fire-Vector.png
     - .. image:: img/Zelda_Sequential-tile-key-Vector.png
     - .. image:: img/Zelda_Sequential-tile-goal-Vector.png
     - .. image:: img/Zelda_Sequential-tile-spider-Vector.png
     - .. image:: img/Zelda_Sequential-tile-wall-Vector.png
   * - Sprite2D
     - .. image:: img/Zelda_Sequential-tile-avatar-Sprite2D.png
     - .. image:: img/Zelda_Sequential-tile-attack_fire-Sprite2D.png
     - .. image:: img/Zelda_Sequential-tile-key-Sprite2D.png
     - .. image:: img/Zelda_Sequential-tile-goal-Sprite2D.png
     - .. image:: img/Zelda_Sequential-tile-spider-Sprite2D.png
     - .. image:: img/Zelda_Sequential-tile-wall-Sprite2D.png
   * - Block2D
     - .. image:: img/Zelda_Sequential-tile-avatar-Block2D.png
     - .. image:: img/Zelda_Sequential-tile-attack_fire-Block2D.png
     - .. image:: img/Zelda_Sequential-tile-key-Block2D.png
     - .. image:: img/Zelda_Sequential-tile-goal-Block2D.png
     - .. image:: img/Zelda_Sequential-tile-spider-Block2D.png
     - .. image:: img/Zelda_Sequential-tile-wall-Block2D.png


Actions
-------

attack
^^^^^^

:Relative: The actions are calculated relative to the object being controlled.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - attack front


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


remove_sprite
^^^^^^^^^^^^^

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
     Name: Zelda Sequential
     Description: A port of the GVGAI game "Zelda". Pick up keys to reach doors in the correct order and avoid enemies. For example, previously you could go -- key -> door -> door. But now you would need to go -- key -> door --> key --> door.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/newset/floor2.png
     Player:
       AvatarObject: avatar
     Termination:
       Win:
         - eq: [goal:count, 0]
       Lose:
         - eq: [avatar:count, 0]
     Levels:
       - |
         wwwwwwwwwwwww
         w...........w
         w...........w
         w.+...A.....w
         w...........w
         w...........w
         w......3....w
         w.g.........w
         wwwwwwwwwwwww
       - |
         wwwwwwwwwwwww
         w.....+..+g.w
         w.+.w.g.....w
         w.....Ag..3.w
         w+..+.......w
         w....w..+...w
         w.........g.w
         w.3...+++.+ww
         wwwwwwwwwwwww
       - |
         wwwwwwwwwwwwwwwwwwwwwwwwwwwwww
         w........+.............g.....w
         w............................w
         w...A.............3..........w
         w............................w
         w....................3.......w
         w..............wwwwwwwwwwwwwww
         w......................+...g.w
         wwwwwwwwwwwwwwwwwwwwwwwwwwwwww
   Actions:
     # Define action that cannot be controlled by the player.
     # (In this case the spider movement)
     - Name: random_movement
       InputMapping:
         Internal: true
       Behaviours:
         # The gnome and the spider can move into empty space
         - Src:
             Object: spider
             Commands:
               - mov: _dest
               - exec:
                   Action: random_movement
                   Delay: 5
                   Randomize: true
           Dst:
             Object: _empty
         # The spider will not move into the wall, but it needs to keep moving
         - Src:
             Object: spider
             Commands:
               - exec:
                   Action: random_movement
                   Delay: 5
                   Randomize: true
           Dst:
             Object: [wall, key, goal, spider]
         # If the gnome moves into a spider
         - Src:
             Object: spider
           Dst:
             Object: avatar
             Commands:
               - remove: true
               - reward: -1
     # remove the recently spawned attack animation sprite
     - Name: remove_sprite
       InputMapping:
         Internal: true
       Behaviours:
         - Src:
             Object: attack_fire
             Commands:
               - remove: true
           Dst:
             Object: attack_fire
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
             Object: avatar
             Commands:
               - rot: _dir
           Dst:
             Object: avatar
         # Only an avatar with a key can win
         - Src:
             Preconditions:
               - eq: [src.has_key, 1]
             Object: avatar
             Commands:
               - reward: 1
               - decr: has_key
               - mov: _dest
               - set_tile: 0
           Dst:
             Object: goal
             Commands:
               - remove: true
         # If the gnome moves into a gem object, the stick is removed, triggering a win condition
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
               - eq:
                   Arguments: [ src.has_key, 0 ]
                   Commands:
                     - incr: has_key
                     - reward: 1
                     - set_tile: 1
           Dst:
             Object: key
             Commands:
               - eq:
                   Arguments: [ src.has_key, 0 ]
                   Commands:
                     - remove: true
         # If the gnome moves into a spider
         - Src:
             Object: avatar
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: spider
         # The gnome and the spider can move into empty space
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: _empty
     - Name: attack
       InputMapping:
         Inputs:
           1:
             Description: attack front
             OrientationVector: [ -1, 0 ]
             VectorToDest: [-1, 0]
         Relative: true
       Behaviours:
         - Src:
             Object: avatar
             Commands:
               - spawn: attack_fire
           Dst:
             Object: [spider, _empty]
             Commands:
               - remove: true
   Objects:
     - Name: avatar
       Z: 3
       MapCharacter: A
       Variables:
         - Name: has_key
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/swordman1_0.png
           - Image: gvgai/oryx/swordmankey1_0.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 0.5, 0.5]
             Scale: 0.75
           - Shape: triangle
             Color: [0.3, 0.5, 0.2]
             Scale: 1.0
     - Name: attack_fire
       Z: 1
       InitialActions:
         - Action: remove_sprite
           Delay: 3
       MapCharacter: x
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/fire1.png
         Block2D:
           - Shape: square
             Color: [1.0, 0.0, 0.0]
             Scale: 0.5
     - Name: key
       Z: 2
       MapCharacter: "+"
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/key2.png
         Block2D:
           - Shape: triangle
             Color: [0.5, 1.0, 0.5]
             Scale: 0.7
     - Name: goal
       Z: 2
       MapCharacter: g
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/doorclosed1.png
         Block2D:
           - Shape: square
             Color: [0.0, 0.7, 0.0]
             Scale: 0.7
     #   - Name: chaser
     #     Z: 2
     #     MapCharacter: "3"
     #     Observers:
     #       Sprite2D:
     #         - Image: gvgai/oryx/skeleton1.png
     - Name: spider
       Z: 2
       InitialActions:
         - Action: random_movement
           Delay: 5
       MapCharacter: "3"
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/spider1.png
         Block2D:
           - Shape: triangle
             Color: [0.9, 0.1, 0.1]
             Scale: 0.5
     - Name: wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - TilingMode: WALL_16
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
           - Shape: square
             Color: [0.7, 0.7, 0.7]
             Scale: 1.0


