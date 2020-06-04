
.. |red-check| image:: img/red-checkmark-16.png


----------

.. #/properties/Environment
.. _#/properties/Environment:

Environment
===========

:Description: Contains the definition of how the player interacts with the environment and how it is percieved by the players
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - object
     - ``Environment``

:Example:
.. parsed-literal::

   Environment:
     :ref:`Name<#/properties/Environment/properties/Name>`: ... 
     :ref:`TileSize<#/properties/Environment/properties/TileSize>`: ... 
     :ref:`BackgroundTile<#/properties/Environment/properties/BackgroundTile>`: ... 
     :ref:`Player<#/properties/Environment/properties/Player>`: ... 
     :ref:`Termination<#/properties/Environment/properties/Termination>`: ... 
     :ref:`Levels<#/properties/Environment/properties/Levels>`: ... 

:Properties:
.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`Name <#/properties/Environment/properties/Name>`
     - |red-check|
   * - :ref:`TileSize <#/properties/Environment/properties/TileSize>`
     - 
   * - :ref:`BackgroundTile <#/properties/Environment/properties/BackgroundTile>`
     - 
   * - :ref:`Player <#/properties/Environment/properties/Player>`
     - 
   * - :ref:`Termination <#/properties/Environment/properties/Termination>`
     - 
   * - :ref:`Levels <#/properties/Environment/properties/Levels>`
     - 


----------

.. #/properties/Environment/properties/Name
.. _#/properties/Environment/properties/Name:

Name
====

:Description: The name of the environment
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - string
     - ``Name``


----------

.. #/properties/Environment/properties/TileSize
.. _#/properties/Environment/properties/TileSize:

TileSize
========

:Description: The common size of all the tiles in the environment. If tile images are supplied with different sizes to this, they will be scaled to this size
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Default Value**
   * - integer
     - ``TileSize``
     - ``10``


----------

.. #/properties/Environment/properties/BackgroundTile
.. _#/properties/Environment/properties/BackgroundTile:

Background Tile
===============

:Description: Filename of an the image that will be used as a background tile
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - string
     - ``BackgroundTile``


----------

.. #/properties/Environment/properties/Player
.. _#/properties/Environment/properties/Player:

Player
======

:Description: Defines how players (algorithms, agents, humans) interact with the environment.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - object
     - ``Player``

:Example:
.. parsed-literal::

   Player:
     :ref:`Mode<#/properties/Environment/properties/Player/properties/Mode>`: ... 
     :ref:`Actions<#/properties/Environment/properties/Player/properties/Action>`: ... 
     :ref:`Observer<#/properties/Environment/properties/Player/properties/Observer>`: ... 

:Properties:
.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`Mode <#/properties/Environment/properties/Player/properties/Mode>`
     - 
   * - :ref:`Actions <#/properties/Environment/properties/Player/properties/Action>`
     - 
   * - :ref:`Observer <#/properties/Environment/properties/Player/properties/Observer>`
     - 


----------

.. #/properties/Environment/properties/Player/properties/Mode
.. _#/properties/Environment/properties/Player/properties/Mode:

The Mode schema
===============

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Allowed Values**
     - **Default Value**
   * - string
     - ``Mode``
     - ``SINGLE``, ``RTS``, ``MULTI``
     - ``MULTI``


----------

.. #/properties/Environment/properties/Player/properties/Action
.. _#/properties/Environment/properties/Player/properties/Action:

The Action schema
=================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - object
     - ``Actions``

:Example:
.. parsed-literal::

   Actions:
     :ref:`DirectControl<#/properties/Environment/properties/Player/properties/Action/properties/Mode>`: ... 

:Properties:
.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`DirectControl <#/properties/Environment/properties/Player/properties/Action/properties/Mode>`
     - 


----------

.. #/properties/Environment/properties/Player/properties/Action/properties/Mode
.. _#/properties/Environment/properties/Player/properties/Action/properties/Mode:

Direct Control
==============

:Description: Direct all actions to a particular object in the environment.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - string
     - ``DirectControl``


----------

.. #/properties/Environment/properties/Player/properties/Observer
.. _#/properties/Environment/properties/Player/properties/Observer:

The Observer schema
===================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - object
     - ``Observer``

:Example:
.. parsed-literal::

   Observer:
     :ref:`RotateWithAvatar<#/properties/Environment/properties/Player/properties/Observer/properties/RotateWithAvatar>`: ... 
     :ref:`TrackAvatar<#/properties/Environment/properties/Player/properties/Observer/properties/TrackAvatar>`: ... 
     :ref:`Height<#/properties/Environment/properties/Player/properties/Observer/properties/Height>`: ... 
     :ref:`Width<#/properties/Environment/properties/Player/properties/Observer/properties/Width>`: ... 
     :ref:`OffsetX<#/properties/Environment/properties/Player/properties/Observer/properties/OffsetX>`: ... 
     :ref:`OffsetY<#/properties/Environment/properties/Player/properties/Observer/properties/OffsetY>`: ... 

:Properties:
.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`RotateWithAvatar <#/properties/Environment/properties/Player/properties/Observer/properties/RotateWithAvatar>`
     - 
   * - :ref:`TrackAvatar <#/properties/Environment/properties/Player/properties/Observer/properties/TrackAvatar>`
     - 
   * - :ref:`Height <#/properties/Environment/properties/Player/properties/Observer/properties/Height>`
     - 
   * - :ref:`Width <#/properties/Environment/properties/Player/properties/Observer/properties/Width>`
     - 
   * - :ref:`OffsetX <#/properties/Environment/properties/Player/properties/Observer/properties/OffsetX>`
     - 
   * - :ref:`OffsetY <#/properties/Environment/properties/Player/properties/Observer/properties/OffsetY>`
     - 


----------

.. #/properties/Environment/properties/Player/properties/Observer/properties/RotateWithAvatar
.. _#/properties/Environment/properties/Player/properties/Observer/properties/RotateWithAvatar:

The RotateWithAvatar schema
===========================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Default Value**
   * - boolean
     - ``RotateWithAvatar``
     - ``False``


----------

.. #/properties/Environment/properties/Player/properties/Observer/properties/TrackAvatar
.. _#/properties/Environment/properties/Player/properties/Observer/properties/TrackAvatar:

The TrackAvatar schema
======================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Default Value**
   * - boolean
     - ``TrackAvatar``
     - ``False``


----------

.. #/properties/Environment/properties/Player/properties/Observer/properties/Height
.. _#/properties/Environment/properties/Player/properties/Observer/properties/Height:

The Height schema
=================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - integer
     - ``Height``


----------

.. #/properties/Environment/properties/Player/properties/Observer/properties/Width
.. _#/properties/Environment/properties/Player/properties/Observer/properties/Width:

The Width schema
================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - integer
     - ``Width``


----------

.. #/properties/Environment/properties/Player/properties/Observer/properties/OffsetX
.. _#/properties/Environment/properties/Player/properties/Observer/properties/OffsetX:

The OffsetX schema
==================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Default Value**
   * - integer
     - ``OffsetX``
     - ``0``


----------

.. #/properties/Environment/properties/Player/properties/Observer/properties/OffsetY
.. _#/properties/Environment/properties/Player/properties/Observer/properties/OffsetY:

The OffsetY schema
==================

:Description: An explanation about the purpose of this instance.
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Default Value**
   * - integer
     - ``OffsetY``
     - ``0``


----------

.. #/properties/Environment/properties/Termination
.. _#/properties/Environment/properties/Termination:

Termination
===========

:Description: Definition of the termination conditions of the environment.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - object
     - ``Termination``

:Example:
.. parsed-literal::

   Termination:
     :ref:`Lose<#/properties/Environment/properties/Termination/properties/Lose>`: ... 
     :ref:`Win<#/properties/Environment/properties/Termination/properties/Win>`: ... 

:Properties:
.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`Lose <#/properties/Environment/properties/Termination/properties/Lose>`
     - 
   * - :ref:`Win <#/properties/Environment/properties/Termination/properties/Win>`
     - 


----------

.. #/properties/Environment/properties/Termination/properties/Lose
.. _#/properties/Environment/properties/Termination/properties/Lose:

Lose Conditions
===============

:Description: If any of these conditions are met, the player associated with this condition will lose the game.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - array
     - ``Lose``
:Array Items:  :ref:`Termination Conditions <#/properties/Environment/properties/Termination/definitions/terminationCondition>`



----------

.. #/properties/Environment/properties/Termination/properties/Win
.. _#/properties/Environment/properties/Termination/properties/Win:

Win Conditions
==============

:Description: If any of these conditions are met, the player associated with this condition will win the game.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - array
     - ``Win``
:Array Items:  :ref:`Termination Conditions <#/properties/Environment/properties/Termination/definitions/terminationCondition>`



----------

.. #/properties/Environment/properties/Termination/definitions/terminationCondition
.. _#/properties/Environment/properties/Termination/definitions/terminationCondition:

Termination Conditions
======================

:Description: When a termination condition is met, the game will reset itself. If there are multiple players, the termination arguments are expanded internally "per player". This can be used to find the first player to a certain number of objects, or the first player to reach a certain score
.. list-table::

   * - **Data Type**
   * - object

:Example:
.. parsed-literal::

   Lose:
     - :ref:`eq<#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/eq>`: ... 
     - :ref:`gt<#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/gt>`: ... 
     - :ref:`lt<#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/lt>`: ... 

   Win:
     - :ref:`eq<#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/eq>`: ... 
     - :ref:`gt<#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/gt>`: ... 
     - :ref:`lt<#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/lt>`: ... 

:Properties:
.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`eq <#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/eq>`
     - 
   * - :ref:`gt <#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/gt>`
     - 
   * - :ref:`lt <#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/lt>`
     - 


----------

.. #/properties/Environment/properties/Termination/definitions/terminationCondition/properties/eq
.. _#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/eq:

Equals
======

:Description: Check if the arguments are equal
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Max Items**
     - **Min Items**
   * - array
     - ``eq``
     - 2
     - 2
:Array Items:  :ref:`Termination Arguments <#/properties/Environment/properties/Termination/definitions/terminationCondition/definitions/terminationArgument>`



----------

.. #/properties/Environment/properties/Termination/definitions/terminationCondition/properties/gt
.. _#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/gt:

Greater Than
============

:Description: Check if the first argument is greater than the second
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Max Items**
     - **Min Items**
   * - array
     - ``gt``
     - 2
     - 2
:Array Items:  :ref:`Termination Arguments <#/properties/Environment/properties/Termination/definitions/terminationCondition/definitions/terminationArgument>`



----------

.. #/properties/Environment/properties/Termination/definitions/terminationCondition/properties/lt
.. _#/properties/Environment/properties/Termination/definitions/terminationCondition/properties/lt:

Less Than
=========

:Description: Check if the first argument is less than the second
.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Max Items**
     - **Min Items**
   * - array
     - ``lt``
     - 2
     - 2
:Array Items:  :ref:`Termination Arguments <#/properties/Environment/properties/Termination/definitions/terminationCondition/definitions/terminationArgument>`



----------

.. #/properties/Environment/properties/Termination/definitions/terminationCondition/definitions/terminationArgument
.. _#/properties/Environment/properties/Termination/definitions/terminationCondition/definitions/terminationArgument:

Termination Arguments
=====================

:Description: An argument to the termination condition. If there are multiple players, then these arguments expand internally as "per player"
:Possible Values:
.. list-table::

   * - **Value**
     - **Description**
   * - ``\w+:count``
     - Returns the number of objects of the type [object]
   * - ``_max_steps``
     - Returns the number of game ticks that have passed
   * - ``_score``
     - The current score
   * - ``[integer]``
     - Any Integer value
.. list-table::



----------

.. #/properties/Environment/properties/Levels
.. _#/properties/Environment/properties/Levels:

Game Level Maps
===============

:Description: Level Strings which define the levels in the game environment.
.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - array
     - ``Levels``
