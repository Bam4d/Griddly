Cook Me Pasta
=============

Description
-------------

Help the chef create the meal, but make sure the ingredients are put together in the right order.

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
            - 14x11
     - .. thumbnail:: img/Cook_Me_Pasta-level-Vector-0.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Sprite2D-0.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Block2D-0.png
   * - .. list-table:: 

          * - Level ID
            - 1
          * - Size
            - 14x11
     - .. thumbnail:: img/Cook_Me_Pasta-level-Vector-1.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Sprite2D-1.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Block2D-1.png
   * - .. list-table:: 

          * - Level ID
            - 2
          * - Size
            - 14x11
     - .. thumbnail:: img/Cook_Me_Pasta-level-Vector-2.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Sprite2D-2.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Block2D-2.png
   * - .. list-table:: 

          * - Level ID
            - 3
          * - Size
            - 14x11
     - .. thumbnail:: img/Cook_Me_Pasta-level-Vector-3.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Sprite2D-3.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Block2D-3.png
   * - .. list-table:: 

          * - Level ID
            - 4
          * - Size
            - 14x11
     - .. thumbnail:: img/Cook_Me_Pasta-level-Vector-4.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Sprite2D-4.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Block2D-4.png
   * - .. list-table:: 

          * - Level ID
            - 5
          * - Size
            - 14x11
     - .. thumbnail:: img/Cook_Me_Pasta-level-Vector-5.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Sprite2D-5.png
     - .. thumbnail:: img/Cook_Me_Pasta-level-Block2D-5.png

Code Example
------------

The most basic way to create a Griddly Gym Environment. Defaults to level 0 and SPRITE_2D rendering.

.. code-block:: python


   import gym
   import griddly

   if __name__ == '__main__':

       env = gym.make('GDY-Cook-Me-Pasta-v0')
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
     - wall
     - key
     - lock
     - boiling_water
     - raw_pasta
     - tomato
     - tuna
   * - Map Char ->
     - `A`
     - `w`
     - `k`
     - `l`
     - `b`
     - `p`
     - `o`
     - `t`
   * - Vector
     - .. image:: img/Cook_Me_Pasta-tile-avatar-Vector.png
     - .. image:: img/Cook_Me_Pasta-tile-wall-Vector.png
     - .. image:: img/Cook_Me_Pasta-tile-key-Vector.png
     - .. image:: img/Cook_Me_Pasta-tile-lock-Vector.png
     - .. image:: img/Cook_Me_Pasta-tile-boiling_water-Vector.png
     - .. image:: img/Cook_Me_Pasta-tile-raw_pasta-Vector.png
     - .. image:: img/Cook_Me_Pasta-tile-tomato-Vector.png
     - .. image:: img/Cook_Me_Pasta-tile-tuna-Vector.png
   * - Sprite2D
     - .. image:: img/Cook_Me_Pasta-tile-avatar-Sprite2D.png
     - .. image:: img/Cook_Me_Pasta-tile-wall-Sprite2D.png
     - .. image:: img/Cook_Me_Pasta-tile-key-Sprite2D.png
     - .. image:: img/Cook_Me_Pasta-tile-lock-Sprite2D.png
     - .. image:: img/Cook_Me_Pasta-tile-boiling_water-Sprite2D.png
     - .. image:: img/Cook_Me_Pasta-tile-raw_pasta-Sprite2D.png
     - .. image:: img/Cook_Me_Pasta-tile-tomato-Sprite2D.png
     - .. image:: img/Cook_Me_Pasta-tile-tuna-Sprite2D.png
   * - Block2D
     - .. image:: img/Cook_Me_Pasta-tile-avatar-Block2D.png
     - .. image:: img/Cook_Me_Pasta-tile-wall-Block2D.png
     - .. image:: img/Cook_Me_Pasta-tile-key-Block2D.png
     - .. image:: img/Cook_Me_Pasta-tile-lock-Block2D.png
     - .. image:: img/Cook_Me_Pasta-tile-boiling_water-Block2D.png
     - .. image:: img/Cook_Me_Pasta-tile-raw_pasta-Block2D.png
     - .. image:: img/Cook_Me_Pasta-tile-tomato-Block2D.png
     - .. image:: img/Cook_Me_Pasta-tile-tuna-Block2D.png


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


YAML
----

.. code-block:: YAML

   Version: "0.1"
   Environment:
     Name: Cook Me Pasta
     Description: Help the chef create the meal, but make sure the ingredients are put together in the right order.
     Observers:
       Sprite2D:
         TileSize: 24
         BackgroundTile: gvgai/newset/floor6.png
     Player:
       AvatarObject: avatar # The player can only control a single avatar in the game
     Termination:
       Win:
         - eq: [complete_meal:count, 1]
       Lose:
         - eq: [wrong_place:count, 1]
     Levels:
       - |
         wwwwwwwwwwwwww
         w............w
         w............w
         w..p......o..w
         w............w
         w......A.....w
         w............w
         w............w
         w..b......t..w
         w............w
         wwwwwwwwwwwwww
       - |
         wwwwwwwwwwwwww
         w............w
         w............w
         w..p......o..w
         w............w
         w......A.....w
         w............w
         w............w
         w..b......t..w
         w............w
         wwwwwwwwwwwwww
       - |
         wwwwwwwwwwwwww
         wA....ww.....w
         w.....ww.....w
         w..o..ww..t..w
         w.....ww.....w
         www.wwwwwwlwww
         w.....ww.....w
         w.....ww.....w
         w..b......p..w
         w....kww.....w
         wwwwwwwwwwwwww
       - |
         wwwwwwwwwwwwww
         w.....w......w
         w..b..w...o..w
         w............w
         wwww....w....w
         w......Aw....w
         wwwww...wwwwww
         w..pw.......kw
         w...l.....t..w
         w...w....w...w
         wwwwwwwwwwwwww
       - |
         wwwwwwwwwwwwww
         w......w.....w
         w...t..w...o.w
         w....wwwww...w
         w............w
         w......A.....w
         w..wwww......w
         w....kw......w
         w..b..wwwwlw.w
         w.....wp.....w
         wwwwwwwwwwwwww
       - |
         wwwwwwwwwwwwww
         w..lA........w
         w..wwwwww.woww
         w..t.........w
         w..wwwwwwww..w
         w..w....k.w..w
         w..w..p...w..w
         w..w...wwww..w
         w..w...b.....w
         w..w.....w...w
         wwwwwwwwwwwwww

   Actions:
     # Define the move action
     - Name: move
       Behaviours:
         # The agent can move around freely in empty space and over holes
         - Src:
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: [boiling_water, raw_pasta, tomato, tuna, cooked_pasta, pasta_sauce]
             Commands:
               - cascade: _dest
         - Src:
             Object: [avatar, boiling_water, raw_pasta, tomato, tuna, cooked_pasta, pasta_sauce]
             Commands:
               - mov: _dest
           Dst:
             Object: _empty

         # Behaviour for boiling_water
         - Src:
             Object: boiling_water
             Commands:
               - remove: true
               - reward: 4
           Dst:
             Object: raw_pasta
             Commands:
               - change_to: cooked_pasta 

         # Behaviour for raw_pasta
         - Src:
             Object: raw_pasta
             Commands:
               - remove: true
               - reward: 4
           Dst:
             Object: boiling_water
             Commands:
               - change_to: cooked_pasta 
      
         # Behaviours for tomato
         - Src:
             Object: tomato
             Commands:
               - remove: true
               - reward: 4
           Dst:
             Object: tuna
             Commands:
               - change_to: pasta_sauce 
            
         # Behaviours for tuna
         - Src:
             Object: tuna
             Commands:
               - remove: true
               - reward: 4
           Dst:
             Object: tomato
             Commands:
               - change_to: pasta_sauce 
      
         # Behaviours for cooked_pasta
         - Src:
             Object: cooked_pasta
             Commands:
               - remove: true
               - reward: 17
           Dst:
             Object: pasta_sauce
             Commands:
               - change_to: complete_meal
         # Behaviours for pasta_sauce
         - Src:
             Object: pasta_sauce
             Commands:
               - remove: true
               - reward: 17
           Dst:
             Object: cooked_pasta
             Commands:
               - change_to: complete_meal 
      
         # If the wrong things are mixed together
         - Src:
             Object: [raw_pasta, boiling_water]
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: [tuna, tomato, pasta_sauce]
             Commands:
               - change_to: wrong_place 
  
         - Src:
             Object: [tuna, tomato, pasta_sauce]
             Commands:
               - remove: true
               - reward: -1
           Dst:
             Object: [boiling_water, raw_pasta]
             Commands:
               - change_to: wrong_place 
      

         # Keys and Locks
         - Src:
             Preconditions:
               - eq: [has_key, 1]
             Object: avatar
             Commands:
               - mov: _dest
           Dst:
             Object: lock
             Commands:
               - remove: true

         # Avatar picks up the key
         - Src: 
             Object: avatar
             Commands:
               - mov: _dest
               - incr: has_key
           Dst:
             Object: key
             Commands:
               - remove: true

   Objects:

     - Name: avatar
       MapCharacter: A
       Variables:
         - Name: has_key
       Observers:
         Sprite2D:
           - Image: gvgai/newset/chef.png
         Block2D:
           - Shape: square
             Color: [0.3, 0.8, 0.3]
             Scale: 0.8

     - Name: wall
       MapCharacter: w
       Observers:
         Sprite2D:
           - Image: gvgai/newset/floor4.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.2, 0.6]
             Scale: 0.8

     - Name: key
       MapCharacter: k
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/key2.png
         Block2D:
           - Shape: square
             Color: [0.2, 0.2, 0.6]
             Scale: 0.8

     - Name: lock
       MapCharacter: l
       Observers:
         Sprite2D:
           - Image: gvgai/newset/lock1.png
         Block2D:
           - Shape: square
             Color: [0.6, 0.6, 0.6]
             Scale: 0.8

     - Name: boiling_water
       MapCharacter: b
       Observers:
         Sprite2D:
           - Image: gvgai/newset/boilingwater.png
         Block2D:
           - Shape: triangle
             Color: [0.2, 0.2, 0.2]
             Scale: 0.8
     - Name: raw_pasta
       MapCharacter: p
       Observers:
         Sprite2D:
           - Image: gvgai/newset/pasta.png
         Block2D:
           - Shape: triangle
             Color: [0.6, 0.6, 0.2]
             Scale: 0.3
     - Name: tomato
       MapCharacter: o
       Observers:
         Sprite2D:
           - Image: gvgai/newset/tomato.png
         Block2D:
           - Shape: triangle
             Color: [0.6, 0.2, 0.2]
             Scale: 0.3
     - Name: tuna
       MapCharacter: t
       Observers:
         Sprite2D:
           - Image: gvgai/newset/tuna.png
         Block2D:
           - Shape: triangle
             Color: [0.2, 0.2, 0.6]
             Scale: 0.3

     - Name: cooked_pasta
       Observers:
         Sprite2D:
           - Image: gvgai/newset/pastaplate.png
         Block2D:
           - Shape: triangle
             Color: [0.6, 0.6, 0.6]
             Scale: 0.7
     - Name: pasta_sauce
       Observers:
         Sprite2D:
           - Image: gvgai/newset/tomatosauce.png
         Block2D:
           - Shape: triangle
             Color: [0.6, 0.0, 0.2]
             Scale: 0.7

     - Name: complete_meal
       Observers:
         Sprite2D:
           - Image: gvgai/newset/pastasauce.png
         Block2D:
           - Shape: triangle
             Color: [0.6, 0.0, 0.2]
             Scale: 0.7

     - Name: wrong_place
       Observers:
         Sprite2D:
           - Image: gvgai/oryx/slash1.png
         Block2D:
           - Shape: square
             Color: [1.0, 0.0, 0.0]
             Scale: 1.0


