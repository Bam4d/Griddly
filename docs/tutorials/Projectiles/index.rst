.. _doc_tutorials_projectiles:

############
Projectiles
############

Pew Pew! Sometimes in games we want to create objects that move across the environment under their own power. In this tutorial we learn how to do this using GDY.

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

  - Name: fire_shoot
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

We can do this by defining an action which we will call ``fire_shoot``. In this action, we have 4 ``Input`` objects, with ids 1-4 (0 is reserved for NOP actions).

Each Input is associated with a particular vector, which defines the direction and magnitude of the action. In this particular case the 4 actions correspond to ``up``, ``right``, ``down`` and ``left`` respectively.

Behaviours
==========

We define which objects can perform actions and which objects can be the destination of actions by using ``Behaviours``.

In our snippet above we only have a single ``Behaviour``. This definition says that: If the object ``jelly`` has ``grass`` or ``_empty`` (a special object name for "an empty space") 
in the `destination location` of the action, then ``spawn`` a ``flame`` object there.

The `destination location` is calculated as the location of the `source object` (``jelly``) plus the vector given in ``VectorToDest``.

The action ``fire_shoot`` will automatically be exposed as an ``action_type`` with 4 ``action_ids`` in the environment's action space. 

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

In the previous section, we defined the the ``fire_shoot`` action. In each defined ``action_id`` of the ``InputMapping`` of this action, we include the ``VectorToDest`` and also the ``MetaData`` of this action.
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

fire_projectile_movement
------------------------

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



**********************
Projectile Collisions
**********************




**********************
Full Code Example
**********************

`Full code examples can be found here! <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Projectiles>`_