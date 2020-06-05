.. _#/properties/Environment/properties/Player:

.. #/properties/Environment/properties/Player

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
     :ref:`ActionConfiguration<#/properties/Environment/properties/Player/properties/Action>`: ... 
     :ref:`Observer<#/properties/Environment/properties/Player/properties/Observer>`: ... 

:Properties:

.. list-table::

   * - **Property**
     - **Required**
   * - :ref:`Mode <#/properties/Environment/properties/Player/properties/Mode>`
     - 
   * - :ref:`ActionConfiguration <#/properties/Environment/properties/Player/properties/Action>`
     - 
   * - :ref:`Observer <#/properties/Environment/properties/Player/properties/Observer>`
     - 


.. toctree:: 
   :hidden:

   Mode/index
   ActionConfiguration/index
   Observer/index
