.. _doc_action_spaces:

#############
Action Spaces
#############

********
Overview
********

Griddly provides a common interface for action spaces in python which can be access using:

.. code-block:: python

  env = gym.make('GDY-[your game here]-v0')
  
  # This contains a description of the action space
  env.action_space

All actions follow the following format:

.. code-block:: python

  action = [

      # (Only required if there is no avatar)
      x, # X coordinate of action to perform. 
      y, # Y coordinate of action to perform.

      # (Only required if there is more than one action type)
      action_type, # The type of action to perform (move, gather, attack etc...., 

      # (Always required)
      action_id, # The ID of the action (These are defined by InputMapping in GDY)
  ]

  env.step(action)

All values in this array are integers.


:x, y:
    These coordinates are required when the environment does not specify that there is an avatar to control. The coordinates chosen become the location of the action that will be performed.

    For example in a game like chess, or checkers, the coordinates would correspond to the piece that the player wants to move.

:action_type:
  The action type refers to the index of the action type as defined in the GDY. For example `move`, `gather`, `push` etc...

  A list of the registered (and correctly ordered for use in actions) types can be found using ``env.gdy.get_action_names()``.

:action_id:
  The action id is commonly used for the "direction" component of the action. The action_id directly corresponds to the ``InputMapping`` of the action. 

.. note:: if no ``InputMapping`` is set for an action, a default of 4 action ids is applied. These action ids resolve to "UP", "DOWN", "LEFT" and "RIGHT"

.. note:: All action types include action_id `0` which corresponds to a no-op
  

Sampling
========

Sampling the action space is the same as any other environment:

:env.action_space.sample():
  This will always produce the correct format of actions for the environment that is loaded.


Sampling Valid Actions
======================

In many environment, certain actions may have no effects at all, for example moving an avatar into an immovable object such as a wall. Or attacking a tile that has no objects. 

Griddly provides some helper methods for reducing the action spaces to only sample valid actions and produce masks for calculating valid policies

:env.game.get_available_actions(player_id):
  Returns a dict of locations of objects that can be controlled and the actions that can be used at those locations
  
.. warning:: player_id=0 is reserved for NPCs and internal actions

:env.game.get_available_action_ids(location, action_names):
  Returns a dict of available action_ids at the given location for the given action_names.

ValidActionSpaceWrapper
-----------------------

In order to easily support games with large action spaces such as RTS games, several helper functions are included a wrapper ``ValidActionSpaceWrapper``. The ``ValidActionSpaceWrapper`` has two functions:

- Sampling actions using this wrapper only returns valid actions in the environment. 
- Two helper functions are available to create action masks which can be applied during neural network training to force the network to choose only valid actions.

:env.get_unit_location_mask(player_id, mask_type='full'):
  Returns a mask of all the locations in the grid which can be selected by a particular player.

  If ``mask_type == 'full'`` then a mask of dimensions (grid_height, grid_width) is returned. This mask can be used in the case where a one-hot representation of the entire grid is used for location selection. 

  If ``mask_type == 'reduced'`` then two masks are returned. One for ``grid_height`` and one for ``grid_width``. This mask can be used when two seperate one-hot representations are used for ``x`` and ``y`` selection.

.. warning:: player_id=0 is reserved for NPCs and internal actions

:env.get_unit_action_mask(location, action_names, padded=True):
  Returns a mask for the ``action_type`` and and ``action_id``

  If ``padded == True`` all masks will be returned with the length padded to the size of the largest number of action ids across all the actions.

  If ``padded == False`` all masks are returned with the length of the number of action ids per action.

.. code-block:: python

    env.reset() # Wrapper must be applied after the reset

    env = ValidActionSpaceWrapper(env)

    unit_location_mask = env.get_unit_location_mask(player_id, mask_type='full')
    unit_action_mask = env.get_unit_action_mask(location, action_names, padded=True)




.. seealso:: A Closer Look at Action Masking in Policy Gradient Algorithms: https://arxiv.org/abs/2006.14171

Valid Action Trees
------------------

Valid action trees can be used to construct Conditional Action Trees, which can be used to iteratively apply masks to complex action spaces depending on the previous actions selected.

:env.game.build_valid_action_trees():
  Returns a valid action tree for the current state for each player.

.. seealso:: You can find several examples of Conditional Action Trees being used with Griddly and RLLib here: https://github.com/Bam4d/conditional-action-trees

********
Examples
********

In this section we break down some example action spaces. In all Griddly environments, ``env.action_space.sample()`` can be used to see what valid action spaces look like.

Here are some explanations of valid actions in different environments are and how to use them.

Single Player
=============

Single Action Type
------------------

If the environment has a single action type then only the ``action_id`` needs to be sent to ``env.step``.

This is usually the case in environments where there is an avatar that can only be moved and there are no special actions defined like ``attack`` or ``pick_up``.

Assuming that our only ``action_type`` in the environment is ``move`` then the following code can be used to move the avatar in a particular direction:

.. code-block:: python

  # env.step(action_id)
  # OR env.step([action_id])

  env.step(3) # Move the avatar right 
  env.step(1) # Move the avatar left


Multiple Action Types
---------------------

In the case where there may be a more complicated action space, for example if there is an avatar that can "move", but also "attack" in any direction around it, the ``action_type`` and ``action_id`` must both be supplied.

For example:

.. code-block:: python

  # env.step([action_type, action_id])

  env.step([0, 3]) # Move the avatar right 
  env.step([1, 1]) # Attack to the left of the avatar

Multi-Agent
===========

Multiple Player Actions
-----------------------

In multi-agent environments, ``env.step`` expects a list of actions for all players. To send actions to individual players in a call to ``env.step``, set ``action_id = 0`` for any of the players that are not performing an action.

for example:

.. code-block:: python

  env.step([
    1, # Action for player 1
    0 # Action for player 2 (which is a no-op)
  ])


Single Action Type
------------------

If there is only a single action type available, a list of ``action_id`` values can be sent directly to ``env.step`` 

.. code-block:: python
  
  env.step([
    1, # Action for player 1
    2 # Action for player 2
  ])

Multiple Action Types
---------------------

If there are multiple action types available, ``env.step`` must contain a list of values for each player giving the ``action_type`` and ``action_id``:

Given that there are two action types "move" and "attack" and each action type has default ``InputMapping``, the following code can be used to send "move left" to player 1 and "attack forward" to player 2.

.. code-block:: python
  
  env.step([
    [0, 1], # Action for player 1 (move left)
    [1, 2]  # Action for player 2 (attack forward)
  ])


Real Time Strategy (RTS)
========================

Multiple players, Multiple Action Types, Action Coordinates
-----------------------------------------------------------

In RTS games, multiple actions for multiple players can be performed in single time-steps. 

Lets say our RTS game has units that have an action ``move`` and an action ``gather`` (to gather resources). Leta also say that there are three units for each player. We can control them in one call to ``env.step``.

.. code-block:: python

  # env.step([
  #   [ # List of actions for player 1
  #     [x1, y1, action_type1, action_id1],
  #     [x2, y2, action_type2, action_id2],
  #     ...
  #   ], 
  #   [ # List of actions for player 2
  #     [x1, y1, action_type1, action_id1],
  #     [x2, y2, action_type2, action_id2],
  #     ..
  #   ],
  # ])

  env.step([
    # Player 1
    [ 
      [3, 10, 0, 3], # Move the unit at [3,10] right
      [4, 7, 1, 1], # The unit at [4,7] will gather resources in front of it
      [4, 4, 0, 0] # The unit at [4, 4] will do nothing. (this can also be ommitted with the same effect) 
    ],

    # Player 2
    [
      [10, 4, 1, 3], # The unit at [10,4] will gather resources to the right
      [13, 2, 1, 1] # The unit at [13,2] will gather resources to the left
    ]
  ])

