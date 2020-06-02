.. _doc_tutorials_gdy:

GDY Tutorial
============

.. note:: It's recommended to follow the short :ref:`GDY Schema tutorial <_doc_tutorials_gdy_schema>` which will help validate any issues with GDY files as they are being created.

This tutorial will take you through building a simple game with GDY. At the end of this tutorial you will have recreated the `"minigrid" <https://github.com/maximecb/gym-minigrid>` environment in Griddly.

The tutorial comes in three steps:

1. Define all the objects that might exist in the environment and how they will be rendered on screen.
2. Define the mechanics of the environment (known as `actions` in GDY). This is how the different objects interact with one another
3. Define how the player (or players) interact with the environment and design the levels.

Afterwards the environment can be loaded by Griddy and used in other projects. In this tutorial the environment will be loaded into an OpenAI gym wrapper and can be played with using your keyboard.

.. toctree::
   :maxdepth: 1
   :name: toc-gdy-tutorial

   objects
   actions
   environment
   gym