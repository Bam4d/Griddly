.. _#/properties/Environment/properties/Termination/definitions/terminationConditionV2/properties/Conditions:

.. #/properties/Environment/properties/Termination/definitions/terminationConditionV2/properties/Conditions

Conditions
==========

:Description: If any of these conditions are met, the game will end and distribute rewards to the associated players.

.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - array
     - ``Conditions``


:Array Type:

.. list-table::

   * - **Type**
     - **Description**
   * -  :ref:`Termination Conditions V1<#/properties/Environment/properties/Termination/definitions/terminationConditionV1>` 
     - When a termination condition is met, the game will reset itself. If there are multiple players, the termination arguments are expanded internally "per player". This can be used to find the first player to a certain number of objects, or the first player to reach a certain score


.. toctree:: 
   :maxdepth: 5
   :hidden:

   /reference/GDY/Environment/Termination/terminationConditionV1/index
