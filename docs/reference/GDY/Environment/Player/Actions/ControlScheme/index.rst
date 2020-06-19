.. _#/properties/Environment/properties/Player/properties/Action/properties/ControlScheme:

.. #/properties/Environment/properties/Player/properties/Action/properties/ControlScheme

Control Mapping
===============

:Description: The control scheme used to process actions.

:Possible Values:

.. list-table::

   * - **Value**
     - **Type**
     - **Description**
   * - ``SELECTION_ABSOLUTE``
     - string
     - Maps RTS style actions, actions 1,2,3,4 map the destination object to the object on the left, up, right and down respectively. The source location is selected by the user and does not track a particular object
   * - ``SELECTION_RELATIVE``
     - string
     - Maps RTS style actions but relative to objects being controlled. Action Ids 1 and 3 (left and right) set the destination of the action as the source. The rotation is relative to the rotation of the object that is being directed. Only action 2 (up) sets the destination of the action to the object in front of the directed object.. The source location is selected by the user and does not track a particular object
   * - ``DIRECT_RELATIVE``
     - string
     - Action Ids 1 and 3 (left and right) set the destination of the action as the source. The rotation is relative to the rotation of the object that is being directed. Only action 2 (up) sets the destination of the action to the object in front of the directed object.
   * - ``DIRECT_ABSOLUTE``
     - string
     - Actions 1,2,3,4 map the destination object to the object on the left, up, right and down respectively. The source object is always the object defined by the DirectControl option.


.. list-table::

   * - **Data Type**
     - **YAML Key**
     - **Default Value**
   * - string
     - ``ControlScheme``
     - ``SELECTION_ABSOLUTE``


