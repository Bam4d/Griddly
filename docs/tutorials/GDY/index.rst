.. _doc_tutorials_gdy:

#############################
GDY Tutorial - Making Sokoban
#############################

This tutorial will take you through building the game Sokoban using Griddly's Game Description Yaml (GDY).

The tutorial comes in three stages covering the main areas of the GDY configuration file ``Enviroment``, ``Actions`` and ``Objects``.

.. code-block::
   
   Version: 0.1
   Enviroment: ...

   Actions: ...

   Objects: ...

Choose from the following options to learn how to configure each section:

:ref:`Enviroment <doc_tutorials_gdy_environment>` -  Define how the player (or players) interact with the environment and design the levels.

:ref:`Actions <doc_tutorials_gdy_actions>` - Define the mechanics of the environment. This is how the different objects interact with one another.

:ref:`Objects <doc_tutorials_gdy_objects>` - Define all the objects that might exist and how they will be rendered on screen.

Finally you can load the game in python and play it:

--> :ref:`How to play the tutorial game <doc_tutorials_gdy_gym>`


.. toctree::
   :maxdepth: 1
   :name: toc-gdy-tutorial
   :hidden:

   objects
   actions
   environment
   gym