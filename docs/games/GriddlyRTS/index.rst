GriddlyRTS
==========

Description
-------------

An RTS Game. There's aliens and stuff.

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
            - 30x30
     - .. thumbnail:: img/GriddlyRTS-level-Vector-0.png
     - .. thumbnail:: img/GriddlyRTS-level-Sprite2D-0.png
     - .. thumbnail:: img/GriddlyRTS-level-Block2D-0.png
     - .. thumbnail:: img/GriddlyRTS-level-Isometric-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 30x30
     - .. thumbnail:: img/GriddlyRTS-level-Vector-1.png
     - .. thumbnail:: img/GriddlyRTS-level-Sprite2D-1.png
     - .. thumbnail:: img/GriddlyRTS-level-Block2D-1.png
     - .. thumbnail:: img/GriddlyRTS-level-Isometric-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 57x58
     - .. thumbnail:: img/GriddlyRTS-level-Vector-2.png
     - .. thumbnail:: img/GriddlyRTS-level-Sprite2D-2.png
     - .. thumbnail:: img/GriddlyRTS-level-Block2D-2.png
     - .. thumbnail:: img/GriddlyRTS-level-Isometric-2.png

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

       env = gym.make('GDY-GriddlyRTS-v0')
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
           'GriddlyRTS-Adv',
           'RTS/GriddlyRTS.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
       )

       env = gym.make('GDY-GriddlyRTS-Adv-v0')
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
     - minerals
     - harvester
     - pusher
     - puncher
     - fixed_wall
     - movable_wall
     - base
   * - Map Char ->
     - M
     - H
     - P
     - p
     - W
     - w
     - B
   * - Vector
     - .. image:: img/GriddlyRTS-tile-minerals-Vector.png
     - .. image:: img/GriddlyRTS-tile-harvester-Vector.png
     - .. image:: img/GriddlyRTS-tile-pusher-Vector.png
     - .. image:: img/GriddlyRTS-tile-puncher-Vector.png
     - .. image:: img/GriddlyRTS-tile-fixed_wall-Vector.png
     - .. image:: img/GriddlyRTS-tile-movable_wall-Vector.png
     - .. image:: img/GriddlyRTS-tile-base-Vector.png
   * - Sprite2D
     - .. image:: img/GriddlyRTS-tile-minerals-Sprite2D.png
     - .. image:: img/GriddlyRTS-tile-harvester-Sprite2D.png
     - .. image:: img/GriddlyRTS-tile-pusher-Sprite2D.png
     - .. image:: img/GriddlyRTS-tile-puncher-Sprite2D.png
     - .. image:: img/GriddlyRTS-tile-fixed_wall-Sprite2D.png
     - .. image:: img/GriddlyRTS-tile-movable_wall-Sprite2D.png
     - .. image:: img/GriddlyRTS-tile-base-Sprite2D.png
   * - Block2D
     - .. image:: img/GriddlyRTS-tile-minerals-Block2D.png
     - .. image:: img/GriddlyRTS-tile-harvester-Block2D.png
     - .. image:: img/GriddlyRTS-tile-pusher-Block2D.png
     - .. image:: img/GriddlyRTS-tile-puncher-Block2D.png
     - .. image:: img/GriddlyRTS-tile-fixed_wall-Block2D.png
     - .. image:: img/GriddlyRTS-tile-movable_wall-Block2D.png
     - .. image:: img/GriddlyRTS-tile-base-Block2D.png
   * - Isometric
     - .. image:: img/GriddlyRTS-tile-minerals-Isometric.png
     - .. image:: img/GriddlyRTS-tile-harvester-Isometric.png
     - .. image:: img/GriddlyRTS-tile-pusher-Isometric.png
     - .. image:: img/GriddlyRTS-tile-puncher-Isometric.png
     - .. image:: img/GriddlyRTS-tile-fixed_wall-Isometric.png
     - .. image:: img/GriddlyRTS-tile-movable_wall-Isometric.png
     - .. image:: img/GriddlyRTS-tile-base-Isometric.png


Actions
-------

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


spawn_harvester
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


punch
^^^^^

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


build_harvester
^^^^^^^^^^^^^^^

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
     Name: GriddlyRTS
     Description: An RTS Game. There's aliens and stuff.
     Observers:
       Sprite2D:
         TileSize: 16
         BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_floor_panel_metal_a.png
       Isometric:
         TileSize: [32, 48]
         BackgroundTile: oryx/oryx_iso_dungeon/floor-1.png
         IsoTileHeight: 16
         IsoTileDepth: 4
     Variables:
       - Name: player_resources
         InitialValue: 0
         PerPlayer: true
     Player:
       Count: 2
     Termination:
       Lose:
         - eq: [base:count, 0] # If the player has no bases
       Win:
         - eq: [_score, 10] # First player to 10 reward points
     Levels:
       - |
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W 
         W  .  .  B1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  M  M  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  M  .  M  M  M  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  M  .  .  M  .  M  M  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  B2 .  .  W 
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W 
         W  .  .  B1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  M  M  .  M  M  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  M  .  M  M  M  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W 
         W  W  W  W  W  W  W  W  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W 
         W  .  .  .  .  .  .  W  W  .  .  .  w  w  w  w  w  w  .  .  .  .  .  W  W  W  W  W  w  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  w  w  .  .  .  .  .  .  .  W  W  W  W  W  w  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  w  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  W  W  W  W  W  w  W 
         W  .  .  .  .  P1 w  .  .  .  .  .  .  .  .  M  M  M  M  M  M  .  W  W  W  W  W  W  W  W 
         W  .  .  .  .  P1 w  .  .  .  .  .  .  .  M  .  .  M  .  M  M  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  P2 w  .  .  .  .  .  .  .  M  M  M  M  M  M  M  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  P2 w  .  .  .  .  .  .  .  .  .  M  M  M  M  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  P1 w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  B2 .  .  W 
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W 
         W  .  .  .  .  M  M  M  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  M  M  M  M  W 
         W  .  .  .  .  .  M  M  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  M  M  W 
         W  .  .  .  .  .  .  M  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  H2 .  .  M  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  B2 H2 .  M  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W 
         W  W  W  w  w  W  W  W  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  .  .  W  W  W  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  W  W  W  W  W  W  W  W  w  w  w  w  w  w  w  w  w  w  w  w  w  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  M  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  M  M  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  M  M  M  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  M  M  M  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W 
         W  W  W  W  W  W  W  W  W  W  W  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  W  W  W  W  W  W  W  W  W  W  W 
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  M  M  M  M  M  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  M  M  M  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  M  M  M  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  M  M  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  M  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  M  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  w  w  w  w  w  w  w  w  w  w  w  w  w  W  W  W  W  W  W  W  W  W  W  W  W  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  W  W  W  .  .  W  W  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  w  w  W  W  W  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  M  .  H1 B1 .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W
         W  M  .  .  H1 .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  M  .  .  .  .  .  .  W
         W  M  M  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  M  M  .  .  .  .  .  W
         W  M  M  M  M  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  M  M  M  .  .  .  .  W
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W

   Actions:
     - Name: spawn_harvester
       InputMapping:
         Internal: true
       Behaviours:
         - Src:
             Object: base
             Commands:
               - spawn: harvester
           Dst:
             Object: _empty
         

         - Src:
             Object: base
           Dst:
             Object: [base, puncher, harvester, pusher, movable_wall]
             Commands:
               - exec:
                   Action: spawn_harvester
                   Delay: 1
                   Randomize: true

     # Harvester costs 5 resources to build
     - Name: build_harvester
       Behaviours:
         - Src:
             Preconditions:
               - gt: [player_resources, 5]
             Object: base
           Dst:
             Object: base
             Commands:
               - exec:
                   Action: spawn_harvester
                   Delay: 10
                   Randomize: true
          
                  
     - Name: gather
       Behaviours:
         - Src:
             Object: harvester
             Commands:
               - incr: resources
               - reward: 1
           Dst:
             Object: minerals
             Commands:
               - decr: resources
               - eq:
                   Arguments: [resources, 0]
                   Commands:
                     - remove: true
         - Src:
             Object: harvester
             Commands:
               - decr: resources
               - reward: 1
           Dst:
             Object: base
             Commands:
               - incr: player_resources

     - Name: move
       Behaviours:
         - Src:
             Object: [harvester, puncher, pusher, movable_wall]
             Commands:
               - mov: _dest # mov will move the object, _dest is the destination location of the action
           Dst:
             Object: _empty

         - Src:
             Object: pusher
             Commands:
               - mov: _dest # mov will move the object, _dest is the destination location of the action
           Dst:
             Object: [movable_wall, harvester, puncher]
             Commands:
               - cascade: _dest # reapply the same action to the dest location of the action

     - Name: punch
       Behaviours:
         - Src:
             Object: puncher
             Commands:
               - reward: 1
           Dst:
             Object: [puncher, harvester, pusher, base]
             Commands:
               - decr: health
               - eq:
                   Arguments: [0, health]
                   Commands:
                     - remove: true

   Objects:
     - Name: minerals
       MapCharacter: M
       Variables:
         - Name: resources
           InitialValue: 200
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_crystal_green.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 1.0
         Isometric:
           - Image: oryx/oryx_iso_dungeon/minerals-1.png

     - Name: harvester
       MapCharacter: H
       Variables:
         - Name: resources
           InitialValue: 0
         - Name: health
           InitialValue: 10
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_jelly_d1.png
         Block2D:
           - Shape: square
             Color: [0.6, 0.2, 0.2]
             Scale: 0.5
         Isometric:
           - Image: oryx/oryx_iso_dungeon/jelly-1.png

     - Name: pusher
       MapCharacter: P
       Variables:
         - Name: health
           InitialValue: 10
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_crawler_queen_d1.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.2, 0.6]
             Scale: 1.0
         Isometric:
           - Image: oryx/oryx_iso_dungeon/queen-1.png

     - Name: puncher
       MapCharacter: p
       Variables:
         - Name: health
           InitialValue: 5
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_monsters/tg_monsters_beast_d1.png
         Block2D:
           - Color: [0.2, 0.6, 0.6]
             Shape: square
             Scale: 0.8
         Isometric:
           - Image: oryx/oryx_iso_dungeon/beast-1.png

     - Name: fixed_wall
       MapCharacter: W
       Observers:
         Sprite2D:
           - TilingMode: WALL_2 # Will tile walls with two images
             Image:
               - oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img33.png
               - oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img40.png
         Block2D:
           - Color: [0.5, 0.5, 0.5]
             Shape: square
         Isometric:
           - Image: oryx/oryx_iso_dungeon/wall-grey-1.png

     - Name: movable_wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img282.png
         Block2D:
           - Color: [0.8, 0.8, 0.8]
             Shape: square
         Isometric:
           - Image: oryx/oryx_iso_dungeon/crate-1.png

     - Name: base
       MapCharacter: B
       Variables:
         - Name: health
           InitialValue: 10
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img324.png
         Block2D:
           - Color: [0.8, 0.8, 0.3]
             Shape: triangle
         Isometric:
           - Image: oryx/oryx_iso_dungeon/base-1.png


