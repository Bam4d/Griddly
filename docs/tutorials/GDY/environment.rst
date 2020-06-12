.. _doc_tutorials_gdy_environment:

Environment
===========

The Environment object contains the definition of how the game will be controlled by the player, the conditions underwhich the game ends, and the levels that are contained in the game.

Step 1 - Player Configuration
-----------------------------

The ``Player`` configuration 

.. code-block:: YAML

    Player:
        Mode: SINGLE
        Actions:
          DirectControl: avatar 



Step 2 - Termination Conditions
-------------------------------

Termination conditions are the rules which decide when the game episodes are complete, and whether the agent wins or loses.

Rules can be defined seperately for ``Win`` and ``Lose``. In Sokoban, we just have a win condition where the player wins if all of the boxes have fallen into the holes. This is how it is defined in GDY.

.. code-block:: YAML

    Termination:
        Win:
          - eq: [box:count, 0]

We define here that the agent wins in the case that the number of ``box`` objects in the environment reaches 0. The ``:count`` option can be appended to any object name to return the number of any of the objects. 

Step 3 - Levels
---------------

In our Sokoban game we are going to define two levels. The layout of each level is defined by a level string which is made up of the ``MapCharacter`` characters that were defined in the :ref:`Objects <doc_tutorials_gdy_objects>` section of this tutorial.

the dot ``.`` character means that the space in the map in unoccupied.

.. code-block:: YAML

    Levels:
        - |
          wwwwwww
          w..hA.w
          w.whw.w
          w...b.w
          whbb.ww
          w..wwww
          wwwwwww
        - |
          wwwwwwwww
          ww.h....w
          ww...bA.w
          w....w..w
          wwwbw...w
          www...w.w
          wwwh....w
          wwwwwwwww

the two defined levels will look like this when rendered:

.. image:: img/getting-started-level-0.png
.. image:: img/getting-started-level-1.png



Step 4 - Name, TileSize and Background Image
--------------------------------------------

-- code-block:: YAML 

    Name: sokoban
    TileSize: 24
    BackgroundTile: images/gvgai/newset/floor2.png

Here we specify the size of the tiles in pixels ``TileSize``. Also if we want to use an image for the background when there are no objects present we can supply a ``BackgroundTile`` image. The ``BackgroundTile`` is ignored any Observers except from ``Sprite2D``.

Additionally every environment must have a unique ``Name`` to differentiate it from other environments.

Putting it all together
-----------------------

The environment definition with all the parts described looks like this:

.. code-block:: YAML

   Environment:
      Name: sokoban
      TileSize: 24
      BackgroundTile: images/gvgai/newset/floor2.png
      Player:
        Mode: SINGLE # This is only a single player game
        Actions:
          DirectControl: avatar # The player can only control a single avatar in the game
      Termination:
        Win:
          - eq: [box:count, 0] # If there are no boxes left
      Levels:
        - |
          wwwwwww
          w..hA.w
          w.whw.w
          w...b.w
          whbb.ww
          w..wwww
          wwwwwww
        - |
          wwwwwwwww
          ww.h....w
          ww...bA.w
          w....w..w
          wwwbw...w
          www...w.w
          wwwh....w
          wwwwwwwww
