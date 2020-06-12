.. _#/properties/Actions/items/properties/Behaviours/items/properties/Src/properties/Preconditions:

.. #/properties/Actions/items/properties/Behaviours/items/properties/Src/properties/Preconditions

Behaviour Preconditions
=======================

:Description: A list of checks that have to be performed before this action is executed.

.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - array
     - ``Preconditions``


:Array Type:

.. list-table::

   * - **Type**
     - **Description**
   * -  :ref:`Behaviour Precondition<#/properties/Actions/items/properties/Behaviours/definitions/behaviourDefinitionPreconditionCommand>` 
     - A check that must be performed before any action. This can be used to change the behaviour of objects based on their internal variables. For example checking whether an object has a key before opening a door.


.. toctree:: 
   :maxdepth: 5
   :hidden:

   /reference/GDY/Actions/items/Behaviours/behaviourDefinitionPreconditionCommand/index
