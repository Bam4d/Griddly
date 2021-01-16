Heal Or Die
===========

Description
-------------

Game environment ported from https://github.com/GAIGResearch/Stratega. 
You have units that heal and units that perform close combat. 
Additionally, on every turn, the health of your units decreases. Win the game by killing your opponents pieces first.


Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - Vector
     - Sprite2D
     - Block2D
     - Isometric
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 25x10
     - .. thumbnail:: img/Heal_Or_Die-level-Vector-0.png
     - .. thumbnail:: img/Heal_Or_Die-level-Sprite2D-0.png
     - .. thumbnail:: img/Heal_Or_Die-level-Block2D-0.png
     - .. thumbnail:: img/Heal_Or_Die-level-Isometric-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 25x19
     - .. thumbnail:: img/Heal_Or_Die-level-Vector-1.png
     - .. thumbnail:: img/Heal_Or_Die-level-Sprite2D-1.png
     - .. thumbnail:: img/Heal_Or_Die-level-Block2D-1.png
     - .. thumbnail:: img/Heal_Or_Die-level-Isometric-1.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly
   from griddly.util.wrappers import InvalidMaskingRTSWrapper

   if __name__ == '__main__':

       env = gym.make('GDY-Heal-Or-Die-v0')
       env.reset()
       env = InvalidMaskingRTSWrapper(env)

       # Replace with your own control algorithm!
       for s in range(1000):
           obs, reward, done, info = env.step(env.action_space.sample())
           for p in range(env.player_count):
               env.render(observer=p) # Renders the environment from the perspective of a single player

           env.render(observer='global') # Renders the entire environment


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
     - `M`
     - `H`
     - `h`
     - `w`
   * - Vector
     - .. image:: img/Heal_Or_Die-tile-mountain-Vector.png
     - .. image:: img/Heal_Or_Die-tile-hole-Vector.png
     - .. image:: img/Heal_Or_Die-tile-healer-Vector.png
     - .. image:: img/Heal_Or_Die-tile-warrior-Vector.png
   * - Sprite2D
     - .. image:: img/Heal_Or_Die-tile-mountain-Sprite2D.png
     - .. image:: img/Heal_Or_Die-tile-hole-Sprite2D.png
     - .. image:: img/Heal_Or_Die-tile-healer-Sprite2D.png
     - .. image:: img/Heal_Or_Die-tile-warrior-Sprite2D.png
   * - Block2D
     - .. image:: img/Heal_Or_Die-tile-mountain-Block2D.png
     - .. image:: img/Heal_Or_Die-tile-hole-Block2D.png
     - .. image:: img/Heal_Or_Die-tile-healer-Block2D.png
     - .. image:: img/Heal_Or_Die-tile-warrior-Block2D.png
   * - Isometric
     - .. image:: img/Heal_Or_Die-tile-mountain-Isometric.png
     - .. image:: img/Heal_Or_Die-tile-hole-Isometric.png
     - .. image:: img/Heal_Or_Die-tile-healer-Isometric.png
     - .. image:: img/Heal_Or_Die-tile-warrior-Isometric.png


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
     Name: Heal Or Die
     Description: |
       Game environment ported from https://github.com/GAIGResearch/Stratega. 
       You have units that heal and units that perform close combat. 
       Additionally, on every turn, the health of your units decreases. Win the game by killing your opponents pieces first.
     Observers:
       Sprite2D:
         TileSize: 16
         BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img125.png
       Isometric:
         TileSize: [64, 64]
         BackgroundTile: stratega/plain.png
         IsoTileHeight: 35
         IsoTileDepth: 0
     Variables:
       - Name: unit_count
         InitialValue: 0
         PerPlayer: true
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
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img355.png
         Block2D:
           - Shape: triangle
             Color: [0.6, 0.7, 0.5]
             Scale: 1.0
         Isometric:
           - Image: stratega/rock.png

     - Name: hole
       MapCharacter: H
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img129.png
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
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_civilian_m_l1.png
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
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png
         Block2D:
           - Color: [0.2, 0.6, 0.6]
             Shape: square
             Scale: 0.8
         Isometric:
           - Image: stratega/basicCloseRange.png


