Push Mania
==========

Description
-------------

Game environment ported from https://github.com/GAIGResearch/Stratega.

Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - Vector
     - Block2D
     - Isometric
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 25x10
     - .. thumbnail:: img/Push_Mania-level-Vector-0.png
     - .. thumbnail:: img/Push_Mania-level-Block2D-0.png
     - .. thumbnail:: img/Push_Mania-level-Isometric-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 25x19
     - .. thumbnail:: img/Push_Mania-level-Vector-1.png
     - .. thumbnail:: img/Push_Mania-level-Block2D-1.png
     - .. thumbnail:: img/Push_Mania-level-Isometric-1.png

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

       env = gym.make('GDY-Push-Mania-v0')
       env.reset()
    
       # Replace with your own control algorithm!
       for s in range(1000):
           for p in range(env.action_space.player_count):
               sampled_action_def = np.random.choice(env.action_space.action_names)
               sampled_action_space = env.action_space.action_space_dict[sampled_action_def].sample()

               action = {
                   'player': p,
                   sampled_action_def: sampled_action_space
               }
               obs, reward, done, info = env.step(action)
            
               env.render(observer=p)

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
           'Push-Mania-Adv',
           'RTS/Stratega/push-mania.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
       )

       env = gym.make('GDY-Push-Mania-Adv-v0')
       env.reset()

       # Replace with your own control algorithm!
       for s in range(1000):
           for p in range(env.action_space.player_count):
               sampled_action_def = np.random.choice(env.action_space.action_names)
               sampled_action_space = env.action_space.action_space_dict[sampled_action_def].sample()

               action = {
                   'player': p,
                   sampled_action_def: sampled_action_space
               }
               obs, reward, done, info = env.step(action)
            
               env.render(observer=p)

           env.render(observer='global')


Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - mountain
     - hole
     - healer
     - warrior
   * - Map Char ->
     - M
     - H
     - h
     - w
   * - Vector
     - .. image:: img/Push_Mania-tile-mountain-Vector.png
     - .. image:: img/Push_Mania-tile-hole-Vector.png
     - .. image:: img/Push_Mania-tile-healer-Vector.png
     - .. image:: img/Push_Mania-tile-warrior-Vector.png
   * - Block2D
     - .. image:: img/Push_Mania-tile-mountain-Block2D.png
     - .. image:: img/Push_Mania-tile-hole-Block2D.png
     - .. image:: img/Push_Mania-tile-healer-Block2D.png
     - .. image:: img/Push_Mania-tile-warrior-Block2D.png
   * - Isometric
     - .. image:: img/Push_Mania-tile-mountain-Isometric.png
     - .. image:: img/Push_Mania-tile-hole-Isometric.png
     - .. image:: img/Push_Mania-tile-healer-Isometric.png
     - .. image:: img/Push_Mania-tile-warrior-Isometric.png


Actions
-------

attack
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


heal
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


unit_counter
^^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - The only action here is to increment the unit count


drain_health
^^^^^^^^^^^^

:Internal: This action can only be called from other actions, not by the player.

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - Reduce the health


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Push Mania
     Description: Game environment ported from https://github.com/GAIGResearch/Stratega.
     Observers:
       # Sprite2D:
       #   TileSize: 24
       #   BackgroundTile:
       Isometric:
         TileSize: [64, 64]
         BackgroundTile: stratega/plain.png
         IsoTileHeight: 35
         IsoTileDepth: 0
     Variables:
       - Name: unit_count
         InitialValue: 0
     Player:
       Count: 2
     Termination:
       Lose:
         - eq: [unit_count, 0] # If the player has no bases
     Levels:
       - |
         M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M
         M  M  M  H  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  .  M  M  M
         M  M  H  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H  M  M
         M  H  .  .  h1 .  M  .  .  .  .  .  H  .  .  .  .  .  M  .  .  .  .  H  M
         M  H  .  .  .  .  M  w1 .  .  .  H  H  H  .  .  .  w2 M  .  h2 .  .  H  M
         M  H  .  .  h1 .  M  w1 .  .  .  H  H  H  .  .  .  w2 M  .  .  .  .  H  M
         M  H  .  .  .  .  M  .  .  .  .  .  H  .  .  .  .  .  M  .  h2 .  .  H  M
         M  M  H  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H  M  M
         M  M  M  H  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  H  M  M  M
         M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M
       - |
         M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M
         M  M  M  H  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  .  M  M  M
         M  M  H  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H  M  M
         M  H  .  .  .  .  M  .  .  .  .  .  H  .  .  .  .  .  M  .  .  .  .  H  M
         M  H  .  .  .  .  M  w1 .  .  .  H  H  H  .  .  .  w2 M  .  .  .  .  H  M
         M  H  .  .  .  .  M  .  .  .  .  H  H  H  .  .  .  .  M  .  .  .  .  H  M
         M  H  .  .  .  .  M  .  .  .  .  .  H  .  .  .  .  .  M  .  .  .  .  H  M
         M  M  H  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H  M  M
         M  M  M  H  h1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  h2 H  M  M  M
         M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M
         M  M  M  H  h1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  h2 H  M  M  M
         M  M  H  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H  M  M
         M  H  .  .  .  .  M  .  .  .  .  .  H  .  .  .  .  .  M  .  .  .  .  H  M
         M  H  .  .  .  .  M  .  .  .  .  H  H  H  .  .  .  .  M  .  .  .  .  H  M
         M  H  .  .  .  .  M  w1 .  .  .  H  H  H  .  .  .  w2 M  .  .  .  .  H  M
         M  H  .  .  .  .  M  .  .  .  .  .  H  .  .  .  .  .  M  .  .  .  .  H  M
         M  M  H  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H  M  M
         M  M  M  H  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  H  M  M  M
         M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M

   Actions:
     # Just a counter for the number of units per player
     - Name: unit_counter
       InputMapping:
         Internal: true
         Inputs:
           1:
             Description: "The only action here is to increment the unit count"
       Behaviours:
         - Src:
             Object: [healer, warrior]
             Commands:
               - incr: unit_count
           Dst:
             Object: [healer, warrior]

     # Reduce all units health by an amount every 10 turns
     - Name: drain_health
       InputMapping:
         Internal: true
         Inputs:
           1:
             Description: "Reduce the health"
       Behaviours:
         - Src:
             Object: [healer, warrior]
             Commands:
               - sub: [health, 25]
               # if the health is 0 then remove the player
               - exec:
                   Action: drain_health
                   ActionId: 1
                   Delay: 50
               - lt:
                   Arguments: [health, 1]
                   Commands:
                     - remove: true
                     - decr: unit_count
           Dst:
             Object: [healer, warrior]

     - Name: move
       Behaviours:
         # Healer and warrior can move in empty space
         - Src:
             Object: [healer, warrior]
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # Healer and warrior can fall into holes
         - Src:
             Object: [healer, warrior]
             Commands:
               - remove: true
               - decr: unit_count
           Dst:
             Object: hole

     - Name: heal
       Behaviours:
         # Healer can heal adjacent warriors and other healers
         - Src:
             # Can only heal units on your own team
             Preconditions:
               - eq: [src._playerId, dst._playerId]
             Object: healer
           Dst:
             Object: [healer, warrior]
             Commands:
               - add: [health, 100]

     - Name: attack
       Behaviours:
         # Warrior can damage adjacent warriors and healers
         - Src:
             # Can only attack units of different players
             Preconditions:
               - neq: [src._playerId, dst._playerId]
             Object: warrior
           Dst:
             Object: [healer, warrior]
             Commands:
               - sub: [health, 25]
               - lt:
                   Arguments: [health, 1]
                   Commands:
                     - remove: true
                     - decr: unit_count

   Objects:
     - Name: mountain
       MapCharacter: M
       Observers:
         # Sprite2D:
         #   - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_crystal_green.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 1.0
         Isometric:
           - Image: stratega/rock.png

     - Name: hole
       MapCharacter: H
       Observers:
         # Sprite2D:
         #   - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_jelly_d1.png
         Block2D:
           - Shape: square
             Color: [0.6, 0.2, 0.2]
             Scale: 0.5
         Isometric:
           - Image: stratega/hole.png

     - Name: healer
       MapCharacter: h
       Variables:
         - Name: health
           InitialValue: 150
       InitialActions:
         - Action: drain_health
           ActionId: 1
           Delay: 50
         - Action: unit_counter
           ActionId: 1
       Observers:
         # Sprite2D:
         #   - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_crawler_queen_d1.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.2, 0.6]
             Scale: 1.0
         Isometric:
           - Image: stratega/healer.png

     - Name: warrior
       MapCharacter: w
       Variables:
         - Name: health
           InitialValue: 200
       InitialActions:
         - Action: drain_health
           ActionId: 1
           Delay: 50
         - Action: unit_counter
           ActionId: 1
       Observers:
         # Sprite2D:
         #   - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png
         Block2D:
           - Color: [0.2, 0.6, 0.6]
             Shape: square
             Scale: 0.8
         Isometric:
           - Image: stratega/basicCloseRange.png


