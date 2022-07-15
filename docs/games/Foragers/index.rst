.. _doc_foragers:

Foragers
========

.. code-block::

   Multi-Agent/foragers.yaml

Description
-------------

A very simple multi-agent game. Agents must collect the coloured potions

Levels
---------

.. list-table:: Levels
   :class: level-gallery
   :header-rows: 1

   * - 
     - Vector
     - Sprite2D
     - Block2D
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 9x10
     - .. thumbnail:: img/Foragers-level-Vector-0.png
     - .. thumbnail:: img/Foragers-level-Sprite2D-0.png
     - .. thumbnail:: img/Foragers-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 9x10
     - .. thumbnail:: img/Foragers-level-Vector-1.png
     - .. thumbnail:: img/Foragers-level-Sprite2D-1.png
     - .. thumbnail:: img/Foragers-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 18x20
     - .. thumbnail:: img/Foragers-level-Vector-2.png
     - .. thumbnail:: img/Foragers-level-Sprite2D-2.png
     - .. thumbnail:: img/Foragers-level-Block2D-2.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Foragers-v0')
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
     - harvester
     - potion1
     - potion2
     - potion3
     - fixed_wall
   * - Map Char ->
     - `f`
     - `b`
     - `r`
     - `g`
     - `W`
   * - Vector
     - .. image:: img/Foragers-tile-harvester-Vector.png
     - .. image:: img/Foragers-tile-potion1-Vector.png
     - .. image:: img/Foragers-tile-potion2-Vector.png
     - .. image:: img/Foragers-tile-potion3-Vector.png
     - .. image:: img/Foragers-tile-fixed_wall-Vector.png
   * - Sprite2D
     - .. image:: img/Foragers-tile-harvester-Sprite2D.png
     - .. image:: img/Foragers-tile-potion1-Sprite2D.png
     - .. image:: img/Foragers-tile-potion2-Sprite2D.png
     - .. image:: img/Foragers-tile-potion3-Sprite2D.png
     - .. image:: img/Foragers-tile-fixed_wall-Sprite2D.png
   * - Block2D
     - .. image:: img/Foragers-tile-harvester-Block2D.png
     - .. image:: img/Foragers-tile-potion1-Block2D.png
     - .. image:: img/Foragers-tile-potion2-Block2D.png
     - .. image:: img/Foragers-tile-potion3-Block2D.png
     - .. image:: img/Foragers-tile-fixed_wall-Block2D.png


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


init_potion
^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - The only action here is to increment the potion count


gather
^^^^^^

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
     Name: Foragers
     Description: A very simple multi-agent game. Agents must collect the coloured potions
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/oryx/grass_15.png
       Block2D:
         TileSize: 24
     Player:
       Count: 4
       Observer:
         TrackAvatar: true
         Height: 5
         Width: 5
         OffsetX: 0
         OffsetY: 0
       AvatarObject: harvester
     Variables:
       - Name: potion_count
         InitialValue: 0
     Termination:
       End:
         - eq: [potion_count, 0]

     Levels:
       - |
         W   W   W   W   W   W   W   W   W
         W   f1  .   .  .    .   .   f2  W
         W   .   .   .   .   .   .   .   W
         W   .   .   r   b   r   .   .   W
         W   .   .   r   g   r   .   .   W
         W   .   .   r   g   r   .   .   W
         W   .   .   r   b   r   .   .   W
         W   .   .   .   .   .   .   .   W
         W   f4  .   .   .   .   .   f3  W
         W   W   W   W   W   W   W   W   W
       - |
         W   W   W   W   W   W   W   W   W
         W   f1  .   .   .   .   .   f2  W
         W   .   W   W   .   W   W   .   W
         W   .   W   r   b   r   W   .   W
         W   .   .   r   g   r   .   .   W
         W   .   W   r   g   r   W   .   W
         W   .   W   r   b   r   W   .   W
         W   .   W   W   .   W   W   .   W
         W   f4  .   .   .   .   .   f3  W
         W   W   W   W   W   W   W   W   W
       - |
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
         W   f1  .   .  .    .   .   .   W   W   .   .   .  .    .   .   f2  W
         W   .   .   .   .   .   .   .   W   W   .   .   .   .   .   .   .   W
         W   .   .   r   b   r   .   .   W   W   .   .   r   b   r   .   .   W
         W   .   .   r   g   r   .   .   W   W   .   .   r   g   r   .   .   W
         W   .   .   r   g   r   .   .   W   W   .   .   r   g   r   .   .   W
         W   .   .   r   b   r   .   .   W   W   .   .   r   b   r   .   .   W
         W   .   .   .   .   .   .   .   W   W   .   .   .   .   .   .   .   W
         W   .   .   .   .   .   .   .   W   W   .   .   .   .   .   .   .   W
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W
         W   .   .   .  .    .   .   .   W   W   .   .   .  .    .   .   .   W
         W   .   .   .   .   .   .   .   W   W   .   .   .   .   .   .   .   W
         W   .   .   r   b   r   .   .   W   W   .   .   r   b   r   .   .   W
         W   .   .   r   g   r   .   .   W   W   .   .   r   g   r   .   .   W
         W   .   .   r   g   r   .   .   W   W   .   .   r   g   r   .   .   W
         W   .   .   r   b   r   .   .   W   W   .   .   r   b   r   .   .   W
         W   .   .   .   .   .   .   .   W   W   .   .   .   .   .   .   .   W
         W   f4  .   .   .   .   .   .   W   W   .   .   .   .   .   .   f3  W
         W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W   W


   Actions:
     - Name: init_potion
       InputMapping:
         Internal: true
         Inputs:
           1:
             Description: "The only action here is to increment the potion count"
       Behaviours:
         - Src:
             Object: [ potion1, potion2, potion3 ]
             Commands:
               - incr: potion_count
           Dst:
             Object: [ potion1, potion2, potion3 ]

     - Name: gather
       Behaviours:
         - Src:
             Object: harvester
             Commands:
               - reward: 1
           Dst:
             Object: [potion1, potion2, potion3]
             Commands:
               - decr: value
               - eq:
                   Arguments: [ value, 0 ]
                   Commands:
                     - decr: potion_count
                     - remove: true

     - Name: move
       Behaviours:
         - Src:
             Object: harvester
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

   Objects:
     - Name: harvester
       MapCharacter: f
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/avatars/man1.png
         Block2D:
           - Shape: triangle
             Color: [ 0.2, 0.2, 0.9 ]
             Scale: 1.0

     - Name: potion1
       MapCharacter: b
       InitialActions:
         - Action: init_potion
           ActionId: 1
       Variables:
         - Name: value
           InitialValue: 5
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/potion-0.png
             Scale: 0.5
         Block2D:
           - Color: [ 0.0, 0.0, 0.8 ]
             Shape: square

     - Name: potion2
       MapCharacter: r
       InitialActions:
         - Action: init_potion
           ActionId: 1
       Variables:
         - Name: value
           InitialValue: 10
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/potion-2.png
             Scale: 0.8
         Block2D:
           - Color: [ 0.8, 0.0, 0.0 ]
             Shape: square

     - Name: potion3
       MapCharacter: g
       InitialActions:
         - Action: init_potion
           ActionId: 1
       Variables:
         - Name: value
           InitialValue: 20
       Observers:
         Sprite2D:
           - Image: oryx/oryx_fantasy/potion-3.png
             Scale: 1.0
         Block2D:
           - Color: [ 0.0, 0.8, 0.0 ]
             Shape: square
             Scale: 0.8

     - Name: fixed_wall
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
           - Color: [ 0.5, 0.5, 0.5 ]
             Shape: square


