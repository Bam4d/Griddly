.. _#/properties/Environment/properties/Player/properties/Action:

.. #/properties/Environment/properties/Player/properties/Action

Action Configuration
====================

:Description: Defines how the players (humans, algorithms, AI) will provide actions to the environment.

.. list-table::

   * - **Data Type**
     - **YAML Key**
   * - object
     - ``ActionConfiguration``



:Example:

.. parsed-literal::

   ActionConfiguration:
     :ref:`DirectControl<#/properties/Environment/properties/Player/properties/Action/properties/Mode>`: ... 

:Properties:

.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`DirectControl <#/properties/Environment/properties/Player/properties/Action/properties/Mode>`
     - 


.. toctree:: 
   :hidden:

   DirectControl/index
