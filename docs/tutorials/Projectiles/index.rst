.. _doc_tutorials_projectiles:

############
Projectiles
############

Pew Pew! Sometimes in games we want to create objects that move across the environment under their own power. In this tutorial we learn how to do this using :ref:`GDY <doc_getting_started_gdy>`.
We build an environment where the ``jelly`` agent can shoot projectiles to break ``box`` es that are sitting on an island in the middle of an ocean of slime.
The agent receives a reward of 1 for every time a projectile hits a box, and a reward of 10 when all boxes are destroyed.

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted height="10%">

          <source src="../../../../../_static/video/tutorials/projectiles/global_video_test.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

There's several game mechanics here that we will explain in detail:

* **Spawning objects**
* **Initial actions**
* **Input mappings**
* **Delayed actions**
* **Internal actions**
* **Collisions**

****************************
Spawning the "flame" object 
****************************

To spawn an object in a particular direction we use the following code:

.. code:: yaml

  - Name: flame_shoot
    InputMapping:
      Inputs:
        1:
          OrientationVector: [ 0, -1 ]
          VectorToDest: [ 0, -1 ]
          MetaData:
            image_idx: 0
        2:
          OrientationVector: [ 1, 0 ]
          VectorToDest: [ 1, 0 ]
          MetaData:
            image_idx: 1
        3:
          OrientationVector: [ 0, 1 ]
          VectorToDest: [ 0, 1 ]
          MetaData:
            image_idx: 2
        4:
          OrientationVector: [ -1, 0 ]
          VectorToDest: [ -1, 0 ]
          MetaData:
            image_idx: 3
    Behaviours:
      - Src:
          Object: jelly
        Dst:
          Object: [ grass, _empty ]
          Commands:
            - spawn: flame

Firstly want to be able to ``spawn`` our pink ``flame`` object in a particular direction from our ``jelly`` agent.

We can do this by defining an action which we will call ``flame_shoot``. In this action, we have 4 ``Input`` objects, with ids 1-4 (0 is reserved for NOP actions).

Each Input is associated with a particular vector, which defines the direction and magnitude of the action. In this particular case the 4 actions correspond to ``up``, ``right``, ``down`` and ``left`` respectively.

Behaviours
==========

We define which objects can perform actions and which objects can be the destination of actions by using ``Behaviours``.

In our snippet above we only have a single ``Behaviour``. This definition says that: If the object ``jelly`` has ``grass`` or ``_empty`` (a special object name for "an empty space") 
in the `destination location` of the action, then ``spawn`` a ``flame`` object there.

The `destination location` is calculated as the location of the `source object` (``jelly``) plus the vector given in ``VectorToDest``.

The action ``flame_shoot`` will automatically be exposed as an ``action_type`` with 4 ``action_ids`` in the environment's action space. 

.. seealso:: You can find much more information about action spaces :ref:`here <doc_action_spaces>`


******************************************************
Setting the ``flame`` tile image and initial direction
******************************************************

There are 4 images that we are going to use for the ``flame`` object:

.. list-table::
   :header-rows: 1

   * - ``tile_id`` 
     - 0
     - 1
     - 2
     - 3
   * - Image
     - .. image:: img/fire-pink-up.png
     - .. image:: img/fire-pink-right.png
     - .. image:: img/fire-pink-down.png
     - .. image:: img/fire-pink-left.png

When the ``flame`` spawns, we want to make sure we set the correct tile based on the direction. For this we can use action ``MetaData`` variables and ``InitialActions``:


Action MetaData
===============

.. code:: yaml

   MetaData:
     image_idx: 0

In the previous section, we defined the the ``flame_shoot`` action. In each defined ``action_id`` of the ``InputMapping`` of this action, we include the ``VectorToDest`` and also the ``MetaData`` of this action.
For each ``action_id`` you can define as many ``MetaData`` variables as you like. Think of them as constants that are available in the behaviour of the action. 
For each of the ``action_ids`` we set a ``image_idx`` variable which we can then use to set the current tile on the ``flame`` object.

In the GDY we define 4 tiles which can be used to render the ``flame`` object:

.. code:: yaml

   Objects:
     - Name: flame
       ... 
       Observers:
         Isometric:
           - Image: oryx/oryx_iso_dungeon/fire-pink-up.png
           - Image: oryx/oryx_iso_dungeon/fire-pink-right.png
           - Image: oryx/oryx_iso_dungeon/fire-pink-down.png
           - Image: oryx/oryx_iso_dungeon/fire-pink-left.png

Now we have defined our 4 images for UP, DOWN, LEFT and RIGHT and our ``image_idx`` for each direction, we can make sure the right image is selected using ``InitialActions`` 

Initial Actions
===============

For this game in particular, we are going to create **two** initial actions. The first will only set the correct tile for the corresponding direction and the second will set the ``flame`` object in motion.


.. code:: yaml

   - Name: flame
     ...
     InitialActions:
       - Action: set_flame_direction
       - Action: flame_projectile_movement
         Delay: 2

set_flame_direction
--------------------

.. code:: yaml
   
   - Name: set_flame_direction
     InputMapping:
       Internal: true
     Behaviours:
       - Src:
           Object: flame
           Commands:
             - set_tile: meta.image_idx
         Dst:
           Object: [ grass, _empty, flame, box ]

When an object is spawned, it automatically inherits the ``MetaData`` and ``VectorToDest`` of the `spawning action` (in this case ``flame_shoot``). 
This means that the `destination location` for the ``Behaviours`` will be calculated relative to the `source object` using the previous ``VectorToDest``. 

For example: 
* The ``jelly`` at :math:`[5,5]` spawns a ``flame`` object using ``action_id`` 2. The `destination location` of the action is :math:`[6,5]`
* The ``flame`` object is spawned at location :math:`[6,5]` 
* The ``flame`` object then executes ``set_flame_direction``. This also uses ``action_id`` 2 from the previous action, meaning the `destination location` will be :math:`[7,5]`

We don't really care what is in location :math:`[7,5]`, so we can set the possible destination objects as any of the possible objects in the environment.

Finally we perform a ``set_tile`` command using the action ``MetaData``. We can reference this variable using the ``meta.`` prefix:

.. code:: yaml
   
   Commands:
     - set_tile: meta.image_idx  


flame_projectile_movement
-------------------------

We add a delay to the ``flame_projectile_movement`` action so that it's only called after 3 game ticks. 

Like the ``set_flame_direction`` this action will inherit the action ``MetaData`` and ``VectorToDest``. 
We don't need the ``MetaData`` in the ``flame_projectile_movement`` action as we have already set the tile, but the ``VectorToDest`` can be used to set the direction of travel of the projectile.

We will cover this in the next section!

********************
Projectile movement
********************

.. code:: yaml
   
   - Name: flame_projectile_movement
     InputMapping:
       Internal: true
     Behaviours:
       - Src:
           Object: flame
           Commands:
             - mov: _dest
             - eq:
                 Arguments: [ range, 0 ]
                 Commands:
                   - remove: true
             - gt:
                 Arguments: [ range, 0 ]
                 Commands:
                   - decr: range
             - exec:
                 Action: flame_projectile_movement
                 Delay: 3
         Dst:
           Object: [ _empty, grass ]

When ``flame_projectile_movement`` is called, we check the `destination location` (using the inherited ``VectorToDest``) of the object to see if there is ``_empty`` or ``grass`` object. If there is, we run some commands.
Lets break these down line by line:

* Firstly move the ``flame`` object to the ``_dest`` variable, which contains the calculated `destination location`. 

  .. code:: yaml

     - mov: _dest


* Next we check a ``range`` variable. This is initialized in the flame object. If the ``range`` variable is 0. We remove the ``flame`` object.

  .. code:: yaml

     - eq:
       Arguments: [ range, 0 ]
       Commands:
         - remove: true

* Then we check the ``range`` variable again, but this time we are looking if its larger than 0. If it `is`, then we decrement the value by 1.

  .. code:: yaml

     - gt:
         Arguments: [ range, 0 ]
         Commands:
           - decr: range

* Finally we call the ``flame_projectile_movement`` function from within itself. But with a delay of 3 game ticks. So the process repeats again!

  .. code:: yaml
     
     - exec:
         Action: flame_projectile_movement
         Delay: 3

Putting all of these commands together, the ``flame`` object moves one square in the initial direction every 3 game ticks. If the ``flame`` object moves more than it's ``range``. Then it will be removed.

However, what happens if the ``flame`` encounters something thats not ``_empty`` or ``grass``? 
What we **want** to happen is that we want the ``flame`` to destroy boxes, we also want to make sure that flames that bump into each other, or go off the edge of the map disappear.

This can be achieved by adding two more ``Behaviours`` that handle these collisions. 

**********************
Projectile Collisions
**********************

.. code:: yaml
   
   Behaviours:
     ...
     - Src:
         Object: flame
         Commands:
           - remove: true
           - reward: 1
       Dst:
         Object: box
         Commands:
           - remove: true
     - Src:
         Object: flame
         Commands:
           - remove: true
       Dst:
         Object: [flame, _boundary]

In the snippet above, we have two ``Behaviours`` the first one executes if the ``flame`` object has the `destination location` of a ``box`` object. In this case, the we remove both the ``flame`` and the ``box`` and give a reward of 1.

The second ``Behaviour`` will remove the flame if it has the `destination location` of another ``flame`` or the ``_boundary`` object (which is a special pseudo object referring to the boundary of the environment.)

**************
Gym Interface
**************

Load the GDY and create a gym environment
=========================================

Loading the environment is super simple, you can just point the ``GymWrapper`` class at the ``projectiles.yaml``:

.. code:: python

   env = GymWrapper('projectiles.yaml', player_observer_type=gd.ObserverType.ISOMETRIC)
   env.reset()

You now have an ``env`` that you can use in Reinforcement Learning or any other experiments.

Action Space
============

So how can we now use this environment? How are the actions that we have defined exposed in the gym interface?

We have defined 4 actions in our GDY: 

* ``move``
  * Move the ``jelly`` (UP,DOWN,LEFT,RIGHT)
  * We didn't actually mention this one in the tutorial above because its super simple, just a single behaviour that uses the ``mov: _dest`` command. and the default InputMapping (UP,DOWN,LEFT,RIGHT)
* ``flame_projectile_movement``
  * Defines the movement of projectiles
* ``flame_shoot``
  * Shoot a projectile in a particular direction (UP,DOWN,LEFT,RIGHT)
* ``set_flame_direction``
  * Defines the movement of projectiles

But we only want to be able to expose the ``move`` and ``flame_shoot`` actions. All actions defined in GDY are exposed by default, so to **stop** an action being exposed we use the following:

.. code:: yaml

   InputMapping:
     Internal: true

This tells the Griddly engine that these actions are only used internally in the game, and cannot be called by an agent.

The actions that are exposed can then be used in the ``env.step`` function:

.. code:: python

   env.step([0, 1]) # move UP
   env.step([0, 2]) # move RIGHT
   env.step([0, 3]) # move DOWN
   env.step([0, 4]) # move LEFT

   env.step([1, 1]) # flame_shoot UP
   env.step([1, 2]) # flame_shoot RIGHT
   env.step([1, 3]) # flame_shoot DOWN
   env.step([1, 4]) # flame_shoot LEFT

.. seealso:: For more information on how Griddly deals with any action space you should look `here <doc_action_spaces>`

And thats about it for this tutorial!

**********************
Full Code Example
**********************

`Full code examples can be found here! <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Projectiles>`_