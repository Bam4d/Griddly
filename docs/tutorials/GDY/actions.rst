.. _doc_tutorials_gdy_actions:

Actions
=======

Actions are the "mechanics" of any Griddly game.

A single action defines what happens between two objects (or sets of objects) in an environment.

The **source** of an action is the object which perfoms a particular action.
The **destination** of an action is the object that is affected by the action.

Lets look at a few examples to make these ideas more concrete!

Step 1 - Movement
--------------------

We are building the game "Sokoban" so we will first define that our ``avatar`` object which we defined before can move around in empty space. To do that we can define our "move" action as follows:

.. code-block:: YAML
   
   Actions:
    # Define the move action
    - Name: move
      Behaviours:
      # The agent can move around freely in empty space and over holes
        - Src:
            Object: avatar
            Commands:
              - mov: _dest
          Dst:
            Object: _empty

We have named the above action "move" and defined a single behaviour. The behaviour object contains the ``Src`` key with the ``Object`` value ``avatar`` meaning that we are defing what happens if the ``avatar`` object performs the "move" action.
We also define the ``Dst`` key with the ``Object`` value ``_empty``. The ``_empty`` keyword is a special keyword that the behaviour only applies when the action is perfomed on a space that has no objects present.

Finally we have ``Commands`` object in the ``Src`` key. The ``Commands`` object contains a list of instructions that will happen to the ``Src`` object if this action is executed. The command we have here is `mov: _dest` which tells the environment to move the object to the destination of the action. The `_dest` keyword is another special keyword used in actions which contains the location of the destination of the action.

For more information about possible commands that can be run on either the ``Src`` or ``Dst`` objects go `here <#/properties/Actions/items/properties/Behaviours/definitions/behaviourDefinitionCommandList>`_


Step 2 - Pushing boxes
-------------------------

To define that we want ``box`` objects to move when the ``avatar`` object moves into them we can add the following code to our ``Behaviours`` list:

.. code-block:: YAML

    # Boxes can move into empty space
    - Src:
        Object: box
        Commands:
            - mov: _dest
      Dst:
        Object: _empty

    # The agent can push boxes
    - Src:
        Object: avatar
        Commands:
            - mov: _dest
      Dst:
        Object: box
        Commands:
            - cascade: _dest

Here we are actually defining two behaviours. The first behaviour is similar to the one in the previous example. We define that the ``box`` object has the mechanic allowing it to move into empty space. 

The second behaviour we define allows the ``avatar`` object to interact with the ``box`` object. The ``mov`` command tells ``avatar`` it can move in the same way it would just moving into empty space. The ``box`` object however now needs to be moved in the same direction as the ``avatar``. This can be achieved by applying the same "move" action again, but on the destination object. ``cascade: _dest`` re-applies the same action on the destination object, which will move the ``box``!

.. note:: We have only allowed the ``box`` object to "move" into empty space. If the ``_dest`` location is not empty, i.e. it contains a ``wall`` object or a ``hole`` object, the command will not be executed. This will stop the previous action also, stopping the ``avatar`` from moving also.

Step 3 - Pushing a box into a hole
-------------------------------------

Now we can push boxes around in empty space, but we have no defined what will happen if we push the ``box`` into a ``hole`` object. We want to reward the player and also remove the ``box`` object.

.. code-block:: YAML
   
   # If a box is moved into a hole remove it
        - Src:
            Object: box
            Commands:
              - remove: true
              - reward: 1
          Dst:
            Object: hole


Most of this behaviour is hopefully self-explanatory by this point. There are two new commands introduced here ``reward: 1`` which gives the player a reward of value 1 and ``remove: true`` which removes the `Src` object.

Putting It All Together
-----------------------

In order to put all these actions together, there is only one minor change to make to the first behaviour. We need to add ``hole`` to the ``Dst`` objects. This adds the ability for avatars to be able to walk on top of ``hole`` objects!

So the entire actions section of the game **Sokoban** looks like this:

.. code-block:: YAML

    Actions:
    # Define the move action
    - Name: move
      Behaviours:
        # The agent can move around freely in empty space and over holes
        - Src:
            Object: avatar
            Commands:
              - mov: _dest
          Dst:
            Object: [_empty, hole]
        
        # Boxes can move into empty space
        - Src:
            Object: box
            Commands:
                - mov: _dest
          Dst:
            Object: _empty

        # The agent can push boxes
        - Src:
            Object: avatar
            Commands:
              - mov: _dest
          Dst:
            Object: box
            Commands:
              - cascade: _dest

        # If a box is moved into a hole remove it
        - Src:
            Object: box
            Commands:
              - remove: true
              - reward: 1
          Dst:
            Object: hole