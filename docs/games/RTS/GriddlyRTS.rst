GriddlyRTS
==========

Description
-------------

No Description

Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - SPRITE_2D
     - BLOCK_2D
   * - 0
     - .. thumbnail:: img/GriddlyRTS-level-SPRITE_2D-0.png
     - .. thumbnail:: img/GriddlyRTS-level-BLOCK_2D-0.png
   * - 1
     - .. thumbnail:: img/GriddlyRTS-level-SPRITE_2D-1.png
     - .. thumbnail:: img/GriddlyRTS-level-BLOCK_2D-1.png

Code Example
------------

.. code-block:: python


   import gym
   import numpy as np
   from griddly import GymWrapperFactory, gd

   if __name__ == '__main__':
       wrapper = GymWrapperFactory()
    
       wrapper.build_gym_from_yaml(
           "ExampleEnv",
           'RTS/basicRTS.yaml',
           level=0
       )

       env = gym.make('GDY-ExampleEnv-v0')
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
   * - SPRITE_2D
     - .. image:: img/GriddlyRTS-object-SPRITE_2D-minerals.png
     - .. image:: img/GriddlyRTS-object-SPRITE_2D-harvester.png
     - .. image:: img/GriddlyRTS-object-SPRITE_2D-pusher.png
     - .. image:: img/GriddlyRTS-object-SPRITE_2D-puncher.png
     - .. image:: img/GriddlyRTS-object-SPRITE_2D-fixed_wall.png
     - .. image:: img/GriddlyRTS-object-SPRITE_2D-movable_wall.png
     - .. image:: img/GriddlyRTS-object-SPRITE_2D-base.png
   * - BLOCK_2D
     - .. image:: img/GriddlyRTS-object-BLOCK_2D-minerals.png
     - .. image:: img/GriddlyRTS-object-BLOCK_2D-harvester.png
     - .. image:: img/GriddlyRTS-object-BLOCK_2D-pusher.png
     - .. image:: img/GriddlyRTS-object-BLOCK_2D-puncher.png
     - .. image:: img/GriddlyRTS-object-BLOCK_2D-fixed_wall.png
     - .. image:: img/GriddlyRTS-object-BLOCK_2D-movable_wall.png
     - .. image:: img/GriddlyRTS-object-BLOCK_2D-base.png


Actions
-------

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


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: GriddlyRTS
     TileSize: 16
     BackgroundTile: oryx/oryx_tiny_galaxy/tg_sliced/tg_world/tg_world_floor_panel_metal_a.png
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

   Actions:
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
         - Src:
             Object: harvester
             Commands:
               - decr: resources
               - reward: 1
           Dst:
             Object: base
             Commands:
               - incr: resources

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
           InitialValue: 10
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_items/tg_items_crystal_green.png
         Block2D:
           - Shape: triangle
             Color: [0.0, 1.0, 0.0]
             Scale: 1.0

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

     - Name: movable_wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img282.png
         Block2D:
           - Color: [0.8, 0.8, 0.8]
             Shape: square

     - Name: base
       MapCharacter: B
       Variables:
         - Name: resources
           InitialValue: 0
         - Name: health
           InitialValue: 10
       Observers:
         Sprite2D:
           - Image: oryx/oryx_tiny_galaxy/tg_sliced/tg_world_fixed/img324.png
         Block2D:
           - Color: [0.8, 0.8, 0.3]
             Shape: triangle


