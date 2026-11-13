.. _#/properties/Actions/items/properties/Behaviours/definitions/behaviourDefinitionCommand/properties/exec/properties/Search/MaxDepth:

.. #/properties/Actions/items/properties/Behaviours/definitions/behaviourDefinitionCommand/properties/exec/properties/Search/MaxDepth

Max Depth
=========

:Description: Budget for the A* search: the maximum number of search nodes (location + orientation) expanded per decision. If the target is not reached within the budget, the first action towards the most promising explored node is used.

.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Default Value**
   * - integer
     - ``MaxDepth``
     - ``10000``


