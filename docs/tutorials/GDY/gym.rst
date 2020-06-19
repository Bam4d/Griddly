.. _doc_tutorials_gdy_gym:

Playing Griddly Games
=====================

In this short tutorial you will learn how to load a GDY file, convert it to an OpenAI Gym  interface and then use the OpenAI Gym interface to play the game with the w,a,s,d keys on your keyboard.


Step 1 - Imports
----------------

To play games with the keyboard using the gym interface, the ``play`` function can be used to wrap a gym environment

The only griddly import thats required is the ``GymWrapperFactory``, this is used to create gym wrappers for any Griddly environments

.. code-block:: python

    import gym
    from gym.utils.play import play

    from griddly import GymWrapperFactory


Step 2 - Load the GDY
---------------------

the ``build_gym_from_yaml`` builds the Griddly environment from the GDY file and loads a particular level.

This can then be loaded by OpenAI gym's ``make`` command. The name of the environment will be ``GDY-[your environment name]-v0``. In this case the environment name will be ``GDY-Sokoban-v0``

.. code-block:: python

    # This is what to use if you want to use OpenAI gym environments
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml('SokobanTutorial', 'sokoban.yaml', level=0)

Step 3 - Play
-------------

All thats left is to play the game!

.. code-block:: python

    # Create the Environment
    env = gym.make(f'GDY-SokobanTutorial-v0')
    
    # Play the game
    play(env, fps=10, zoom=2)