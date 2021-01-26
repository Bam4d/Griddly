.. _doc_getting_started_gym_advanced:

####################
OpenAI Gym Interface 
####################

Games defined with :ref:`GDY <doc_getting_started_gdy>` files can easily be wrapped by OpenAI's gym interface.

The simplest way to use a pre-made environment is to just use the following code:

.. code-block:: python

    import gym
    import griddly

    if __name__ == '__main__':

        env = gym.make('GDY-Sokoban-v0')
        env.reset()

The ``gym.make`` command can also send several Griddly specific parameters to the environment:

.. code-block:: python

    env = gym.make(
        'GDY-Sokoban-v0'
        player_observer_type=gd.ObserverType.VECTOR,
        global_observer_type=gd.ObserverType.VECTOR,
        level=0,
        max_steps=None,
    )

.. seealso:: More examples and a full listing of all the games can be found on the page for each game in :ref:`Games <doc_games>`

********************************
Registering Your Own Environment
********************************

It's easy to register your own GDY files and wrap them with OpenAI gym using the ``GymWrapperFactory``:

.. code-block:: python

    import gym
    from griddly import GymWrapperFactory, gd

    if __name__ == '__main__':
        wrapper = GymWrapperFactory()

        wrapper.build_gym_from_yaml('MyNewEnvironment', 'my_new_env_gdy.yaml')

        env = gym.make('GDY-MyNewEnvironment-v0')
        env.reset()

**************
Observer Types
**************

When generating an environment you can specify how you want the environment to be rendered. You can do this by setting the ``player_observer_type`` and ``global_observer_type`` parameters in the ``gym.make`` function, or the ``build_gym_from_yaml`` function.

.. seealso:: For more information about observation spaces, states and event history see :ref:`Observation Spaces <doc_observation_spaces>`

*******************
The Global Observer
*******************

The global observer can be used alongside any of the other observers and will always render the entire environment regardless of how other observers are defined.
This means that you can pass vector observations to your agents and then render with sprites or blocks to make awesome demos!

.. code-block:: python

    env = gym.make(f'GDY-Sokoban-Adv-v0', global_observer_type=gd.ObserverType.SPRITE_2D)
    env.reset()
    
    env.render(observer='global')