Griddly Description YAML (GDY)
==============================

Here you can find the full documentation of all the options that are available in GDL.

A GDL file contains 4 top-level mappings ``Version``, ``Environment``, ``Actions``, ``Objects``

.. code-block:: YAML

    Version: "0.0"
    Environment: ...

    Objects: ...

    Actions: ...

.. toctree::
   :maxdepth: 1
   :name: toc-gdy

   environment
   objects
   actions

.. jsonschema:: ../../resources/gdy-schema.json#/properties/Version