.. _doc_tutorials_gdy:

#############################
GDY Tutorial - Making Sokoban
#############################

.. note:: It's recommended to follow the short :ref:`GDY Schema tutorial <doc_tutorials_gdy_schema>` which will help validate any issues with GDY files as they are being created.

This tutorial will take you through building a simple game with GDY. At the end of this tutorial you will have recreated the Sokoban game in Griddly.

The tutorial comes in three main stages which represent each of the three main configuration stages, ``Enviroment``, ``Actions`` and ``Objects``.

The top level of GDY files always look like this:

.. code-block::
   
   Version: 0.1
   Enviroment: ...

   Actions: ...

   Objects: ...


:ref:`Enviroment <doc_tutorials_gdy_environment>` -  Define how the player (or players) interact with the environment and design the levels.

:ref:`Actions <doc_tutorials_gdy_actions>` - Define the mechanics of the environment. This is how the different objects interact with one another.

:ref:`Objects <doc_tutorials_gdy_objects>` - Define all the objects that might exist and how they will be rendered on screen.

Afterwards the environment can be loaded by Griddy and used in other projects. In this tutorial the environment will be loaded into an OpenAI gym wrapper and can be played with using your keyboard.

--> :ref:`How to play the tutorial game <doc_tutorials_gdy_gym>`


.. toctree::
   :maxdepth: 1
   :name: toc-gdy-tutorial
   :hidden:

   objects
   actions
   environment
   gym