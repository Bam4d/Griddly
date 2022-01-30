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

By not defining the ``Inputs`` key here, Griddly will use the default ``action_id``s for UP, DOWN, LEFT, RIGHT. Note here we also set the action ``InputMapping`` to ``Internal: true``. This is so the ``cow_random_movement`` action cannot be access by the agent.

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

This action defines what will happen when the ``cow_random_movement`` action is performed on the ``cow`` object when the `destination location` has grass or dirt objects. There are three ``Commands``; the first one will move the cow to the `destination location` of the action, the second will execute the ``cow_eat_grass`` action (we will get to this later). The third will re-execute the ``cow_random_movement`` with a small delay. It will also **randomize** the ``action_id`` that is executed. This means that it will randomly choose UP, DOWN, LEFT or RIGHT for the next action.

What happens if the ``cow`` is not next to ``grass`` or ``dirt``? We don't want to move the cow, but we **do** want to try to move the cow again with a short delay. If we don't do this the cow will get stuck and no longer move!

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







***********************************
Cow eating with random probability
***********************************