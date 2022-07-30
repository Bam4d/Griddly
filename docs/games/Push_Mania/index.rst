.. _doc_push_mania:

Push Mania
==========

.. code-block::

   RTS/Stratega/push-mania.yaml

Description
-------------

Game environment ported from https://github.com/GAIGResearch/Stratega.
You must push all your opponents pieces into the holes.


Levels
---------

.. list-table:: Levels
   :class: level-gallery
   :header-rows: 1

   * - 
     - Sprite2D
     - Vector
     - Block2D
     - Isometric
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 10x11
     - .. thumbnail:: img/Push_Mania-level-Sprite2D-0.png
     - .. thumbnail:: img/Push_Mania-level-Vector-0.png
     - .. thumbnail:: img/Push_Mania-level-Block2D-0.png
     - .. thumbnail:: img/Push_Mania-level-Isometric-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 25x11
     - .. thumbnail:: img/Push_Mania-level-Sprite2D-1.png
     - .. thumbnail:: img/Push_Mania-level-Vector-1.png
     - .. thumbnail:: img/Push_Mania-level-Block2D-1.png
     - .. thumbnail:: img/Push_Mania-level-Isometric-1.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly
   from griddly.util.wrappers import InvalidMaskingRTSWrapper

   if __name__ == '__main__':

       env = gym.make('GDY-Push-Mania-v0')
       env.reset()
       env = InvalidMaskingRTSWrapper(env)

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
     - hole
     - pusher
   * - Map Char ->
     - `H`
     - `p`
   * - Sprite2D
     - .. image:: img/Push_Mania-tile-hole-Sprite2D.png
     - .. image:: img/Push_Mania-tile-pusher-Sprite2D.png
   * - Vector
     - .. image:: img/Push_Mania-tile-hole-Vector.png
     - .. image:: img/Push_Mania-tile-pusher-Vector.png
   * - Block2D
     - .. image:: img/Push_Mania-tile-hole-Block2D.png
     - .. image:: img/Push_Mania-tile-pusher-Block2D.png
   * - Isometric
     - .. image:: img/Push_Mania-tile-hole-Isometric.png
     - .. image:: img/Push_Mania-tile-pusher-Isometric.png


Actions
-------

drain_health
^^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - Reduce the health


push
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


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Push Mania
     Description: |
       Game environment ported from https://github.com/GAIGResearch/Stratega.
       You must push all your opponents pieces into the holes.
     Observers:
       Sprite2D:
         TileSize: 32
         BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img23.png
       Isometric:
         TileSize: [64, 64]
         BackgroundTile: stratega/plain.png
         IsoTileHeight: 35
         IsoTileDepth: 0
       Vector:
         IncludePlayerId: true
         IncludeVariables: true
     Variables:
       - Name: unit_count
         InitialValue: 0
     Player:
       Count: 2
     Termination:
       Lose:
         - eq: [pusher:count, 0] # Player loses its king, it loses the game
     Levels:
       - |
         H  H  H  H  H  H  H  H  H  H
         H  .  p1 .  .  .  .  .  .  H
         H  p1 .  .  .  .  .  .  .  H
         H  .  .  .  .  .  .  .  .  H
         H  .  .  .  H  H  .  .  .  H
         H  .  .  .  H  H  .  .  .  H
         H  .  .  .  H  H  .  .  .  H
         H  .  .  .  .  .  .  .  .  H
         H  .  .  .  .  .  .  .  p2 H
         H  .  .  .  .  .  .  p2 .  H
         H  H  H  H  H  H  H  H  H  H
       - |
         H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H
         H  .  .  .  .  .  .  .  .  .  .  .  .  .  H  .  .  .  .  .  .  .  .  .  H
         H  .  .  H  .  .  .  H  .  .  .  .  .  .  .  .  .  .  .  .  .  H  .  .  H
         H  p1 p1 H  .  .  H  H  H  H  H  H  H  H  H  H  H  H  H  .  .  H  p2 p2 H
         H  .  .  H  .  .  .  .  .  .  .  H  .  .  .  .  .  .  .  .  .  H  .  .  H
         H  H  H  H  .  .  .  .  .  .  .  .  H  .  .  .  .  .  .  .  .  H  H  H  H
         H  .  .  H  .  .  .  .  H  H  H  H  H  H  H  H  H  .  .  .  .  H  .  .  H
         H  p1 p1 H  .  .  .  H  H  H  H  H  H  H  H  H  H  .  .  .  .  H  p2 p2 H
         H  .  .  H  .  .  .  .  .  .  .  .  .  .  H  .  .  .  .  .  .  H  .  .  H
         H  .  .  .  .  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  .  .  .  .  H
         H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H  H

   Actions:

     # Reduce all units health by an amount every 10 turns
     - Name: drain_health
       InputMapping:
         Internal: true
         Inputs:
           1:
             Description: "Reduce the health"
       Behaviours:
         - Src:
             Object: pusher
             Commands:
               - sub: [health, 10]
               # if the health is 0 then remove the player
               - exec:
                   Action: drain_health
                   ActionId: 1
                   Delay: 50
               - lt:
                   Arguments: [health, 1]
                   Commands:
                     - remove: true
                     - reward: -1

           Dst:
             Object: pusher

     - Name: move
       Behaviours:
         # Healer and warrior can move in empty space
         - Src:
             Object: pusher
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # Healer and warrior can fall into holes
         - Src:
             Object: pusher
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: hole

     - Name: push
       Behaviours:
         # Pushers can push other pushers
         - Src:
             Object: pusher
             Commands: 
               - mov: _dest
           Dst:
             Object: pusher
             Commands:
               - cascade: _dest

   Objects:

     - Name: hole
       MapCharacter: H
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img343.png
         Block2D:
           - Shape: square
             Color: [0.6, 0.2, 0.2]
             Scale: 0.5
         Isometric:
           - Image: stratega/hole.png

     - Name: pusher
       MapCharacter: p
       Variables:
         - Name: health
           InitialValue: 150
       InitialActions:
         - Action: drain_health
           ActionId: 1
           Delay: 50
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_astronaut_l1.png
         Block2D:
           - Shape: triangle
             Color: [0.2, 0.6, 0.2]
             Scale: 1.0
         Isometric:
           - Image: stratega/healer.png


