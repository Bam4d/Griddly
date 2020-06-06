.. _#/properties/Environment/properties/Termination/properties/Win:

.. #/properties/Environment/properties/Termination/properties/Win

Win Conditions
==============

:Description: If any of these conditions are met, the player associated with this condition will win the game.

.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - array
     - ``Win``


:Array Type:

.. list-table::

   * - **Type**
     - **Description**
   * -  :ref:`Termination Conditions<#/properties/Environment/properties/Termination/definitions/terminationCondition>` 
     - When a termination condition is met, the game will reset itself. If there are multiple players, the termination arguments are expanded internally "per player". This can be used to find the first player to a certain number of objects, or the first player to reach a certain score


.. toctree:: 
   :maxdepth: 5
   :hidden:

   /reference/GDY/Environment/Termination/terminationCondition/index
