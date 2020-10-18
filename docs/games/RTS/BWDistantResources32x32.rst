BWDistantResources32x32
=======================

Description
-------------

No Description

Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - Sprite2D
     - Block2D
   * - .. list-table:: 

          * - Level ID
            - 0
          * - Size
            - 32x32
     - .. thumbnail:: img/BWDistantResources32x32-level-Sprite2D-0.png
     - .. thumbnail:: img/BWDistantResources32x32-level-Block2D-0.png

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

       env = gym.make('GDY-BWDistantResources32x32-v0')
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
           'BWDistantResources32x32-Adv',
           'RTS/BWDistantResources32x32.yaml',
           level=0,
           global_observer_type=gd.ObserverType.SPRITE_2D,
           player_observer_type=gd.ObserverType.SPRITE_2D,
       )

       env = gym.make('GDY-BWDistantResources32x32-Adv-v0')
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
     - fixed_wall
     - harvester
     - base
   * - Map Char ->
     - M
     - W
     - H
     - B
   * - Sprite2D
     - .. image:: img/BWDistantResources32x32-object-Sprite2D-minerals.png
     - .. image:: img/BWDistantResources32x32-object-Sprite2D-fixed_wall.png
     - .. image:: img/BWDistantResources32x32-object-Sprite2D-harvester.png
     - .. image:: img/BWDistantResources32x32-object-Sprite2D-base.png
   * - Block2D
     - .. image:: img/BWDistantResources32x32-object-Block2D-minerals.png
     - .. image:: img/BWDistantResources32x32-object-Block2D-fixed_wall.png
     - .. image:: img/BWDistantResources32x32-object-Block2D-harvester.png
     - .. image:: img/BWDistantResources32x32-object-Block2D-base.png


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
     Name: BWDistantResources32x32
     Observers:
       Sprite2D:
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
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  M  M  M 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  M  M 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  M 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  B1 .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  W  W  W  W  .  .  .  . 
         .  .  .  .  W  W  W  W  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  B2 .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
         .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
         M  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         M  M  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
         M  M  M  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .

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
             Object: [harvester]
             Commands:
               - mov: _dest # mov will move the object, _dest is the destination location of the action
           Dst:
             Object: _empty

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


