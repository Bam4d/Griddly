Kill The King
=============

Description
-------------

Game environment ported from https://github.com/GAIGResearch/Stratega.
Both you and your opponent must protect the king from being killed.


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
            - 32x17
     - .. thumbnail:: img/Kill_The_King-level-Vector-0.png
     - .. thumbnail:: img/Kill_The_King-level-Sprite2D-0.png
     - .. thumbnail:: img/Kill_The_King-level-Block2D-0.png
     - .. thumbnail:: img/Kill_The_King-level-Isometric-0.png

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

       env = gym.make('GDY-Kill-The-King-v0')
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
           'Kill-The-King-Adv',
           'RTS/Stratega/kill-the-king.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
       )

       env = gym.make('GDY-Kill-The-King-Adv-v0')
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
     - water
     - forest
     - hole
     - healer
     - warrior
     - archer
     - king
   * - Map Char ->
     - M
     - W
     - F
     - H
     - h
     - w
     - a
     - k
   * - Vector
     - .. image:: img/Kill_The_King-tile-mountain-Vector.png
     - .. image:: img/Kill_The_King-tile-water-Vector.png
     - .. image:: img/Kill_The_King-tile-forest-Vector.png
     - .. image:: img/Kill_The_King-tile-hole-Vector.png
     - .. image:: img/Kill_The_King-tile-healer-Vector.png
     - .. image:: img/Kill_The_King-tile-warrior-Vector.png
     - .. image:: img/Kill_The_King-tile-archer-Vector.png
     - .. image:: img/Kill_The_King-tile-king-Vector.png
   * - Sprite2D
     - .. image:: img/Kill_The_King-tile-mountain-Sprite2D.png
     - .. image:: img/Kill_The_King-tile-water-Sprite2D.png
     - .. image:: img/Kill_The_King-tile-forest-Sprite2D.png
     - .. image:: img/Kill_The_King-tile-hole-Sprite2D.png
     - .. image:: img/Kill_The_King-tile-healer-Sprite2D.png
     - .. image:: img/Kill_The_King-tile-warrior-Sprite2D.png
     - .. image:: img/Kill_The_King-tile-archer-Sprite2D.png
     - .. image:: img/Kill_The_King-tile-king-Sprite2D.png
   * - Block2D
     - .. image:: img/Kill_The_King-tile-mountain-Block2D.png
     - .. image:: img/Kill_The_King-tile-water-Block2D.png
     - .. image:: img/Kill_The_King-tile-forest-Block2D.png
     - .. image:: img/Kill_The_King-tile-hole-Block2D.png
     - .. image:: img/Kill_The_King-tile-healer-Block2D.png
     - .. image:: img/Kill_The_King-tile-warrior-Block2D.png
     - .. image:: img/Kill_The_King-tile-archer-Block2D.png
     - .. image:: img/Kill_The_King-tile-king-Block2D.png
   * - Isometric
     - .. image:: img/Kill_The_King-tile-mountain-Isometric.png
     - .. image:: img/Kill_The_King-tile-water-Isometric.png
     - .. image:: img/Kill_The_King-tile-forest-Isometric.png
     - .. image:: img/Kill_The_King-tile-hole-Isometric.png
     - .. image:: img/Kill_The_King-tile-healer-Isometric.png
     - .. image:: img/Kill_The_King-tile-warrior-Isometric.png
     - .. image:: img/Kill_The_King-tile-archer-Isometric.png
     - .. image:: img/Kill_The_King-tile-king-Isometric.png


Actions
-------

warrior_attack
^^^^^^^^^^^^^^

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


archer_attack
^^^^^^^^^^^^^

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


move_2
^^^^^^

.. list-table:: 
   :header-rows: 1

   * - Action Id
     - Mapping
   * - 1
     - 
   * - 2
     - 
   * - 3
     - 
   * - 4
     - 
   * - 5
     - 
   * - 6
     - 
   * - 7
     - 
   * - 8
     - 


move_1
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
     Name: Kill The King
     Description: |
       Game environment ported from https://github.com/GAIGResearch/Stratega.
       Both you and your opponent must protect the king from being killed.
     Observers:
       Sprite2D:
         TileSize: 16
         BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img125.png
       Isometric:
         TileSize: [64, 64]
         BackgroundTile: stratega/plain.png
         IsoTileHeight: 35
         IsoTileDepth: 0
     Player:
       Count: 2
     Termination:
       Lose:
         - eq: [king:count, 0] # Player loses its king, it loses the game
     Levels:
       - |
         M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M  M
         M  .  .  .  .  .  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M
         M  .  k1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M
         M  .  .  h1 a1 w1 .  .  .  .  .  .  .  .  .  .  .  F  F  .  .  .  .  .  .  .  .  .  .  .  .  M
         M  .  .  a1 w1 .  .  .  .  .  .  .  .  .  .  .  F  F  F  .  .  .  .  .  .  .  .  .  .  .  .  M
         M  .  .  w1 .  .  .  .  .  .  .  .  .  .  .  .  .  F  F  .  .  .  M  M  .  .  .  .  .  M  M  M
         M  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  F  .  .  .  .  .  .  .  .  .  .  .  .  M
         M  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M
         M  M  M  .  .  .  .  .  .  .  .  .  .  F  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w2 .  .  M
         M  .  .  .  .  .  .  .  .  .  .  .  F  F  .  .  W  .  .  .  .  .  .  .  .  .  .  w2 a2 .  .  M
         M  .  .  .  .  .  .  .  .  .  .  F  F  F  .  .  W  .  .  .  .  .  .  .  .  .  w2 a2 h2 .  .  M
         M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  .  .  .  .  .  .  .  .  .  .  .  k2 .  M
         M  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  .  .  H  H  H  H  H  .  .  .  .  .  M
         M  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  W  .  .  .  .  .  .  .  .  .  .  .  M
         M  M  M  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  W  W  W  .  .  .  .  .  .  .  .  .  .  M
         M  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  W  W  W  W  W  .  .  .  .  .  .  .  .  .  M
         M  M  M  M  M  M  M  M  M  M  M  W  W  W  W  W  W  W  W  W  M  M  M  M  M  M  M  M  M  M  M  M

   Actions:

     - Name: move_2
       InputMapping:
           Inputs:
             1:
               VectorToDest: [0, 1]
             2:
               VectorToDest: [0, 2]
             3:
               VectorToDest: [0, -1]
             4:
               VectorToDest: [0, -2]
             5:
               VectorToDest: [1, 0]
             6:
               VectorToDest: [2, 0]
             7:
               VectorToDest: [-1, 0]
             8:
               VectorToDest: [-2, 0]
       Behaviours:
         # Healer and warrior can move in empty space
         - Src:
             Object: [warrior, archer, healer]
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # Healer and warrior can fall into holes
         - Src:
             Object: [warrior, archer, healer]
             Commands:
               - remove: true
           Dst:
             Object: hole

     - Name: move_1
       Behaviours:
         # Healer and warrior can move in empty space
         - Src:
             Object: king
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # Healer and warrior can fall into holes
         - Src:
             Object: king
             Commands:
               - remove: true
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
             Object: [healer, warrior, king]
             Commands:
               - add: [health, 10]

     - Name: warrior_attack
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

     - Name: archer_attack
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

   Objects:
     - Name: mountain
       MapCharacter: M
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img355.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 1.0
         Isometric:
           - Image: stratega/rock.png

     - Name: water
       MapCharacter: W
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img185.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 1.0
         Isometric:
           - Image: stratega/water.png

     - Name: forest
       MapCharacter: F
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img332.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 1.0
         Isometric:
           - Image: stratega/forest.png

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
           InitialValue: 40
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_civilian_m_l1.png
         Block2D:
           - Shape: triangle
             Color: [0.7, 0.7, 0.7]
             Scale: 0.5
         Isometric:
           - Image: stratega/healer.png

     - Name: warrior
       MapCharacter: w
       Variables:
         - Name: health
           InitialValue: 200
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png
         Block2D:
           - Color: [0.2, 0.6, 0.2]
             Shape: triangle
             Scale: 0.9
         Isometric:
           - Image: stratega/basicCloseRange.png

     - Name: archer
       MapCharacter: a
       Variables:
         - Name: health
           InitialValue: 100
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_drone_d1.png
         Block2D:
           - Color: [0.2, 0.2, 0.6]
             Shape: triangle
             Scale: 0.9
         Isometric:
           - Image: stratega/basicLongRange.png

     - Name: king
       MapCharacter: k
       Variables:
         - Name: health
           InitialValue: 400
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_lord_l1.png
         Block2D:
           - Color: [0.6, 0.2, 0.2]
             Shape: triangle
             Scale: 1.0
         Isometric:
           - Image: stratega/advancedCloseRange.png


