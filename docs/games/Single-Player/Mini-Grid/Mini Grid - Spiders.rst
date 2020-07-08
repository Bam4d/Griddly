Mini Grid - Spiders
===================

Description
-------------

A port of the games provided in the https://github.com/maximecb/gym-minigrid Dynamic obstacles environment, but you're a gnome avoiding ghosts to get to a gem.

Levels
---------

.. list-table:: Levels
   :header-rows: 1

   * - 
     - SPRITE_2D
     - BLOCK_2D
   * - 0
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-SPRITE_2D-0.png
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-BLOCK_2D-0.png
   * - 1
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-SPRITE_2D-1.png
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-BLOCK_2D-1.png
   * - 2
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-SPRITE_2D-2.png
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-BLOCK_2D-2.png
   * - 3
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-SPRITE_2D-3.png
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-BLOCK_2D-3.png
   * - 4
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-SPRITE_2D-4.png
     - .. thumbnail:: img/Mini_Grid_-_Spiders-level-BLOCK_2D-4.png

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
           'Single-Player/Mini-Grid/minigrid-spiders.yaml',
           level=0
       )

       env = gym.make('GDY-ExampleEnv-v0')
       defined_actions_count = env.defined_actions_count
       env.reset()
    
       # Replace with your own control algorithm!
       for s in range(1000):
           action_id = env.action_space.sample()
           action_definition_id = np.random.randint(env.defined_actions_count)
           obs, reward, done, info = env.step([action_definition_id1, *action_id1])
        
           env.render()

           env.render(observer='global')


Objects
-------

.. list-table:: Tiles
   :header-rows: 2

   * - Name ->
     - wall
     - spider
     - gem
     - gnome
   * - Map Char ->
     - W
     - G
     - g
     - A
   * - SPRITE_2D
     - .. image:: img/Mini_Grid_-_Spiders-object-SPRITE_2D-wall.png
     - .. image:: img/Mini_Grid_-_Spiders-object-SPRITE_2D-spider.png
     - .. image:: img/Mini_Grid_-_Spiders-object-SPRITE_2D-gem.png
     - .. image:: img/Mini_Grid_-_Spiders-object-SPRITE_2D-gnome.png
   * - BLOCK_2D
     - .. image:: img/Mini_Grid_-_Spiders-object-BLOCK_2D-wall.png
     - .. image:: img/Mini_Grid_-_Spiders-object-BLOCK_2D-spider.png
     - .. image:: img/Mini_Grid_-_Spiders-object-BLOCK_2D-gem.png
     - .. image:: img/Mini_Grid_-_Spiders-object-BLOCK_2D-gnome.png


Actions
-------

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

:Relative: The actions are calculated relative to the object being controlled.

:Internal: This action can only be called from other actions, not by the player.

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


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Mini Grid - Spiders
     Description: A port of the games provided in the https://github.com/maximecb/gym-minigrid Dynamic obstacles environment, but you're a gnome avoiding ghosts to get to a gem.
     TileSize: 24
     BackgroundTile: oryx/oryx_fantasy/floor2-2.png
     Player:
       Observer:
         RotateWithAvatar: true
         TrackAvatar: true
         Height: 7
         Width: 7
         OffsetX: 0
         OffsetY: 3
       AvatarObject: gnome
     Termination:
       Win:
         - eq: [gem:count, 0] # If there are no boxes left
       Lose:
         - eq: [gnome:count, 0] # If there are no boxes left
     Levels:
       - |
         W  W  W  W  W  W
         W  A  .  .  .  W
         W  .  .  .  .  W
         W  .  G  .  .  W
         W  .  .  .  g  W
         W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W
         W  A  .  .  .  .  W
         W  .  .  .  .  .  W
         W  .  .  G  .  .  W
         W  .  .  .  .  .  W
         W  .  G  .  .  g  W
         W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  W
         W  .  .  .  .  G  .  W
         W  .  .  .  .  .  .  W
         W  .  G  .  G  .  .  W
         W  .  .  .  .  .  .  W
         W  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  .  .  W
         W  .  .  G  .  .  .  .  .  W
         W  .  .  .  .  .  .  G  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  G  .  .  .  W
         W  .  G  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W  W  W
       - |
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W
         W  A  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  G  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  G  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  G  .  .  .  .  .  G  .  .  .  .  .  G  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  G  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  G  .  .  G  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W
         W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  g  W
         W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W  W

   Actions:
     # Define action that cannot be controlled by the player. (In this case the spider movement)
     - Name: random_movement
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
         Relative: true
         Internal: true
       Behaviours:
         # Spider rotates on the spot
         - Src:
             Object: spider
             Commands:
               - rot: _dir
               - exec:
                   Action: random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: spider

         # The gnome and the spider can move into empty space
         - Src:
             Object: spider
             Commands:
               - mov: _dest
               - exec:
                   Action: random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: _empty

         # The spider will not move into the wall or the gem, but it needs to keep moving
         - Src:
             Object: spider
             Commands:
               - exec:
                   Action: random_movement
                   Delay: 3
                   Randomize: true
           Dst:
             Object: [wall, gem]

         # If the gnome moves into a spider
         - Src:
             Object: spider
           Dst:
             Object: gnome
             Commands:
               - remove: true
               - reward: -1

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
             Object: gnome
             Commands:
               - rot: _dir
           Dst:
             Object: gnome

         # If the gnome moves into a spider
         - Src:
             Object: gnome
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: spider

         # The gnome and the spider can move into empty space
         - Src:
             Object: gnome
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # If the gnome moves into a gem object, the stick is removed, triggering a win condition
         - Src:
             Object: gnome
             Commands:
               - reward: 1
           Dst:
             Object: gem
             Commands:
               - remove: true

   Objects:
     - Name: wall
       MapCharacter: W
       Observers:
         Sprite2D:
           TilingMode: WALL_16
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
           Shape: square
           Color: [0.7, 0.7, 0.7]
           Scale: 1.0

     - Name: spider
       InitialActions:
         - Action: random_movement
           Delay: 3
           Randomize: true
       MapCharacter: G
       Observers:
         Sprite2D:
           Image: oryx/oryx_fantasy/avatars/spider1.png
         Block2D:
           Shape: triangle
           Color: [1.0, 0.0, 0.0]
           Scale: 0.8

     - Name: gem
       MapCharacter: g
       Observers:
         Sprite2D:
           Image: oryx/oryx_fantasy/ore-6.png
         Block2D:
           Shape: triangle
           Color: [0.0, 1.0, 0.0]
           Scale: 0.5

     - Name: gnome
       MapCharacter: A
       Observers:
         Sprite2D:
           Image: oryx/oryx_fantasy/avatars/gnome1.png
         Block2D:
           Shape: triangle
           Color: [0.0, 0.0, 1.0]
           Scale: 0.8


