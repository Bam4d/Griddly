.. _#/properties/Environment/properties/Termination/definitions/terminationConditionV1/properties/neq:

.. #/properties/Environment/properties/Termination/definitions/terminationConditionV1/properties/neq

Not Equals
==========

:Description: Check if the arguments are not equal

.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Max Items**
     - **Min Items**
   * - array
     - ``neq``
     - 2
     - 2


:Array Type:

.. list-table::

   * - **Type**
     - **Description**
   * -  :ref:`Termination Arguments<#/properties/Environment/properties/Termination/definitions/terminationConditionV1/definitions/terminationArgument>` 
     - An argument to the termination condition. If there are multiple players, then these arguments expand internally as "per player"


.. toctree:: 
   :maxdepth: 5
   :hidden:

   /reference/GDY/Environment/Termination/terminationConditionV1/terminationArgument/index
