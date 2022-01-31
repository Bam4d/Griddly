.. _doc_stochasticity:

#############
Stochasticity
#############

In this tutorial you will learn all about how to add stochasticity to environments. We will build an example environment where cows wander around a field and might eat some grass occaisionally. Your job is to replant the grass!

Additionally in this example we use some custom assets from the  `Crafter <https://github.com/danijar/crafter>`_ Reinforcement Learning environment.

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted height="10%">

          <source src="../../_static/video/tutorials/stochasticity/global_video_test.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

There's several game mechanics here that we will explain in detail:

* **Action Execution Probability**
* **Random Action Choices**
* **Initial Actions**
* **Input Mappings**
* **Delayed Actions**
* **Using Custom Assets**
* **Internal Actions**
* **Changing Object Type**


********************
Random Cow Movement
********************

To achieve random movement of any object in Griddly, there are a few components that are required.

Firstly you need an action that defines how the object will move. In our case, we just want the cow to move UP, DOWN, LEFT and RIGHT. The GDY for this is the same as us defining actions for the agent:


.. code:: yaml
   
   - Name: cow_random_movement
     InputMapping:
       Internal: true

By not defining the ``Inputs`` key here, Griddly will use the default ``action_id``s for UP, DOWN, LEFT, RIGHT. Note here we also set the action ``InputMapping`` to ``Internal: true``. 
This is so the ``cow_random_movement`` action cannot be access by the agent.

We then need to define the ``Behaviours`` of the ``cow_random_movement``:

.. code:: yaml

   - Src:
       Object: cow
       Commands:
         - mov: _dest
         - exec:
             Action: cow_eat_grass
         - exec:
             Action: cow_random_movement
             Delay: 1
             Randomize: true
     Dst:
       Object: [ grass, dirt ]

This action defines what will happen when the ``cow_random_movement`` action is performed on the ``cow`` object when the `destination location` has grass or dirt objects. 
There are three ``Commands``; the first one will move the cow to the `destination location` of the action, the second will execute the ``cow_eat_grass`` action (we will get to this later). 
The third will re-execute the ``cow_random_movement`` with a small delay. It will also **randomize** the ``action_id`` that is executed. 
This means that it will randomly choose UP, DOWN, LEFT or RIGHT for the next action.

What happens if the ``cow`` is not next to ``grass`` or ``dirt``? We don't want to move the cow, but we **do** want to try to move the cow again with a short delay. 
If we don't do this the cow will get stuck and no longer move!

.. code:: yaml

   - Src:
       Object: cow
       Commands:
         - exec:
             Action: cow_random_movement
             Delay: 1
             Randomize: true
     Dst:
       Object: [ _empty, _boundary, cow, player ]

Finally, we need to initialize the ``cow_random_movement`` for when the cows are generated at the start of the game. This is done using ``InitialActions``.

.. code:: yaml

   - Name: cow
     InitialActions:
       - Action: cow_random_movement
         Randomize: true


**************************************************
Setting the Probability of a ``cow`` Eating Grass 
**************************************************

Now we need to define the ``cow_eat_grass`` action to only execute a percentage of the time that it is called by the ``cow_random_movement`` command.

.. code:: yaml

   - Name: cow_eat_grass
     Probability: 0.05
     InputMapping:
       Internal: true
       Inputs:
         1:
           VectorToDest: [0, 0]
     Behaviours:
       - Src:
           Object: cow
         Dst:
           Object: grass
           Commands:
             - change_to: dirt

This action contains a ``Probability`` property meaning the action will only be executed with a probability of 0.05 every time it is called. 
When the action **is** executed the ``grass`` object under the ``cow`` will be changed to a ``dirt`` object. 

*************************************************
Planting the ``grass`` that a ``cow`` has Eaten.
*************************************************

The dirt object can then be changed back into ``grass`` by the agent:

.. code:: yaml

   - Name: plant_grass
     InputMapping:
       Inputs:
         1:
           VectorToDest: [ 0, -1 ]
     Behaviours:
       - Src:
           Object: player
           Commands:
             - reward: 1
         Dst:
           Object: dirt
           Commands:
             - change_to: grass

We also restrict the ``plant_grass`` action to the square above the ``player``.


******************************************
Using Custom Assets for your Environments
******************************************

To use custom assets in your environment all you have to do is put all the assets in one directory, and then set the ``image_path`` in the ``GymWrapper`` when creating the environment:

.. code:: python

   env = GymWrapper('stochasticity.yaml',
               player_observer_type=gd.ObserverType.SPRITE_2D,
               global_observer_type=gd.ObserverType.SPRITE_2D,
               image_path='./assets/',
               level=0)

To make sure that your assets look how you expect them. Make sure that they are all the same dimensions. Internally Griddly will re-size them all to the ``TileSize`` (default 24x24) set in the Environment Observer definition:

.. code:: yaml
   
   Environment:
     ...
     Observers:
       Sprite2D:
         TileSize: 48

*****************
Stacking Objects
*****************

In Griddly, objects can be stacked on-top of each other. The stacking order (or Z-index) of the objects must be consistent with the z-index of the objects when they are defined. 
The object on "top" of the stack is always the first object that can be interacted with in actions.

Defining a ``Z`` index of an object is done in the object definition, for example in the following snippet, we define that the ``player`` sprite will always be rendered on top of the ``grass`` sprite:

.. code:: yaml

   - Name: player
     MapCharacter: p
     Z: 2
     Observers:
       Sprite2D:
         - Image: player.png
   - Name: grass
     MapCharacter: G
     Z: 1
     Observers:
       Sprite2D:
         - Image: grass.png


We can also stack objects in the level map itself by using the ``/`` character:

.. code:: yaml

   Levels:
     - |
       G   G   G   G   G   G   G   G
       G   G   G   G   G   G   c/G G
       G   G   G   G   G   G   G   G
       G   G   G   G   G   G   G   G
       G   G   G   p/G G   G   G   G
       G   G   G   G   G   G   G   G
       G   G   G   G   G   G   G   G
       G   c/G G   G   G   G   G   G
       G   G   G   G   G   G   G   G

The Map characters in combination with the ``/`` character have the following meanings:

.. list-table::
   :header-rows: 1

   * - Map Characters 
     - Meaning
   * - ``G``
     -  ``grass``
   * - ``c/G``
     - ``cow`` on ``grass``
   * - ``p/G``
     - ``player`` on ``grass``

**********************
Full Code Example
**********************

`Full code examples can be found here! <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Stochasticity>`_