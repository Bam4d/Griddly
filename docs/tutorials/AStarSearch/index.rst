.. _doc_a_star_search:

##########
A* Search
##########

In this tutorial we will learn how to give objects in the environment a small amount of intelligence by allowing them to use the A-Star Search algorithm to do pathfinding.

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted height="10%">

          <source src="../../_static/video/tutorials/astar/global_video_test.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

We build a simple environment where the agent must find a goal state while being chased by a gross spider. We build two versions of this environment where the ``spider`` has different movement characteristics.

In the first environment the ``spider`` can only move up, down left and right. In the second environment the ``spider`` can only rotate left and right and move forwards.


*************************
Defining Spider Movement
*************************

Firstly we need to define how the ``spider`` moves. In the example where we just want the ``spider`` to be able to move UP, DOWN, LEFT and RIGHT, we can just define a default ``chase`` action:


.. code:: yaml

    - Name: chase
      InputMapping:
        Internal: true

This action will have the default action mapping of UP, DOWN, LEFT and RIGHT. Also note that this action has ``Internal: true`` so that this action cannot be performed by any controlling agents.

Alternatively for the environment where we want our ``spider`` to rotate left and right, and only move in the direction that it is travelling you can do the following:

.. code:: yaml
   
   - Name: chase
     InputMapping:
       Inputs:
         1:
           Description: Rotate left
           OrientationVector: [ -1, 0 ]
         2:
           Description: Move forwards
           OrientationVector: [ 0, -1 ]
           VectorToDest: [ 0, -1 ]
         3:
           Description: Rotate right
           OrientationVector: [ 1, 0 ]
       Relative: true
       Internal: true

.. seealso:: You can find much more information about action spaces :ref:`here <doc_action_spaces>`

****************************
Using the ``Search`` option
****************************

The goal of out environment is to make the ``spider`` object find a path from its current location to a particular destination using the mocements we defined in ``chase``. 
In GDY this is super simple to do and is the same for both of the cases above. You just need to tell the Griddly engine which actions it can use and which objects are `impassable`.

We can do that by using the ``Search`` option in an ``exec`` command.


.. code:: yaml

    - exec:
        Action: chase
        Delay: 10
        Search:
          ImpassableObjects: [ wall ]
          TargetObjectName: catcher

In the Griddly engine, this uses the A* search algorithm to find the best ``actionId`` (in this case which direction) to get the ``spider`` closer to the ``catcher`` object. 
In this case the ``catcher`` object is the name of the avatar we control. We also tell the A* algorithm that you cannot move through ``wall`` objects.

Now all we need to do is make sure the ``exec`` command is called when the ``spider`` moves. We can do that by adding to the ``Behaviours`` of the ``chase`` action:


.. code:: yaml

   - Name: chase
     ...
     Behaviours:
     - Src:
         Object: spider
         Commands:
           - mov: _dest
           - exec:
               Action: chase
               Delay: 10
               Search:
                 ImpassableObjects: [ wall ]
                 TargetObjectName: catcher
       Dst:
         Object: _empty
      
     # We only need this Behaviour if we are using the rotating version of the chase action
     - Src:
         Object: spider
         Commands:
           - rot: _dir 
           - exec:
               Action: chase
               Delay: 0
               Search:
                 ImpassableObjects: [ wall ]
                 TargetObjectName: catcher
       Dst:
         Object: spider

What we are doing here is telling telling the Griddly engine to execute another search operation every time the ``spider`` moves, (or rotates). 
We also only execute the ``chase`` action after a small delay of 10 if the spider actually moves to a new location. If the spider just rotates on the spot, we immediately execute another ``chase`` action so it moves as well as rotates.

*******************
Full Code Example
*******************

`Full code examples can be found here! <https://github.com/Bam4d/Griddly/tree/develop/python/examples/AStar%20Search>`_


