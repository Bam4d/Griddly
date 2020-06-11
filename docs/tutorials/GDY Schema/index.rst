.. _doc_tutorials_gdy_schema:

GDY Schema Tutorial
===================

This tutorial will show you how to set up your IDE to help write GDY files.

The GDY Schema defines all the required and optional properties that can be used to create games. Find your IDE below and follow the instructions to set up the schema validator!

You can either download the `schema <https://raw.githubusercontent.com/Bam4d/Griddly/develop/resources/gdy-schema.json>`_ locally or reference it from the github repository 


Visual Studio Code
------------------

* Install the ``redhat.vscode-yaml`` extension from the visual studio code marketplace.

This extension allows json schema files to be used to add syntax support for yaml files.

* Edit the your workspace settings to add the following lines:

.. code-block:: json
   
   "[yaml]":{
      "editor.insertSpaces": true,
      "editor.tabSize": 2,
      "editor.quickSuggestions": {
        "other": true,
        "comments": false,
        "strings": true
      },
      "editor.autoIndent": "none",
    },
    "yaml.schemas": {
      // "https://raw.githubusercontent.com/Bam4d/Griddly/develop/resources/gdy-schema.json": "[path to your gdy files]/*.yaml"
      "gdy-schema.json": "[path to your gdy files]/*.yaml"
    }

PyCharm
-------

PyCharm has a feature for YAML validation with JSON schema files built in.

In settings navigate to ``Languages And Frameworks -> Schemas and DTDs -> JSON Schema Mappings``

You will need to the schema location to either the schema location on github 

``https://raw.githubusercontent.com/Bam4d/Griddly/develop/resources/gdy-schema.json`` 

or download the schema file locally and point to it on your local machine. 

You will also need to set a `Filepath Pattern` to point to the location you are storing your GDY YAML files.

.. image::img/add_schema_pycharm.png
