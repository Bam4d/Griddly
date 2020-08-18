.. _#/properties/Environment/properties/Termination/definitions/terminationCondition/definitions/terminationArgument:

.. #/properties/Environment/properties/Termination/definitions/terminationCondition/definitions/terminationArgument

Termination Arguments
=====================

:Description: An argument to the termination condition. If there are multiple players, then these arguments expand internally as "per player"

:Possible Values:

.. list-table::

   * - **Value**
     - **Type**
     - **Description**
   * - ``\w+:count``
     - string
     - Returns the number of objects of the object name before the colon, for example ``flower:count`` or ``car:count``
   * - ``_steps``
     - string
     - Returns the number of game ticks that have passed
   * - ``_score``
     - string
     - The current score
   * - ``[integer]``
     - integer
     - Any Integer value


