.. _doc_rllib_intro:

#################################
Reinforcement Learning with RLLib
#################################


Griddly provides support for reinforcement learning using the `RLLib <https://docs.ray.io/en/latest/rllib.html>`_ reinforcement learning library.

While RLLib doesn't support OpenAI Gym registered environments, it does provide a similar interface which is supported by Griddly's ``RLLibEnv`` environment.

Griddly provides two classes, ``RLLibEnv`` and ``RLLibMultiAgentWrapper`` which abstract away all the tedious parts of wrapping environments for RL and leaves you to concentrate on training algorithms, designing networks and game mechanics.


**********************
Environment Parameters
**********************

Parameters for the environments, such as the :ref:`GDY <doc_getting_started_gdy>` file for the game and :ref:`Observer options <doc_observation_spaces>` can be sent to the environment using the ``env_config`` dictionary.

Most of the parameters here are the same as the parameters that can be given to the ``gym.make()`` command when creating a :ref:`Griddly environment for OpenAI Gym <doc_getting_started_gym_advanced>`.

.. code-block:: python

    'env_config': {
        'yaml_file': 'Single-Player/GVGAI/clusters_partially_observable.yaml',
        
        'global_observer_type': gd.ObserverType.SPRITE_2D,
        'record_video_config': {
            'frequency': 100000
        },

        'random_level_on_reset': True,
        'max_steps': 1000,
    },



The above example will also record a video of the environment (rendered using the ``SPRITE_2D`` renderer) for one episode every 100000 steps.
Finally the max_steps of the environment will be override to be 1000 steps before the environment is reset automatically.

*******************
Level Randomization
*******************

Partially observable games have a fixed observations space regardless of the size of the levels. Additionally several games have levels of fixed size.

With these games, the level can be randomized at the end of every episode using the ``random_level_on_reset`` option in the ``env_config`` section of RLLib's config. 

.. code-block:: python

    'env_config': {

        'random_level_on_reset': True,

        ...

If this is set to true then the agent will be placed in one of the random levels described in the GDY file each time the episode restarts.


**********************
Global Average Pooling
**********************

Griddly environments' observation spaces differ between games, levels and visualization options. In order to handle this in a generic way using neural networks, we provide a Global Average Pooling agent `GAPAgent`, which can be used with any 2D environment with no additional configuration.

All you need to do is register the custom model with RLLib and then use it in your training ``config``:

.. code-block:: python

    ModelCatalog.register_custom_model('GAP', GAPAgent)

    ...

    config = {

        'model': {
            'custom_model': 'GAP'
            'custom_model_config': .....
        }
    
        ...

    }



The implementation of the Global Average Pooling agent is essentially a stack of convolutions that maintain the shape of the state,

.. code-block:: python

    class GAPAgent(TorchModelV2, nn.Module):
    """
    Global Average Pooling Agent
    This is the same agent used in https://arxiv.org/abs/2011.06363.

    Global average pooling is a simple way to allow training grid-world environments regardless o the size of the grid.
    """

    def __init__(self, obs_space, action_space, num_outputs, model_config, name):
        super().__init__(obs_space, action_space, num_outputs, model_config, name)
        nn.Module.__init__(self)

        self._num_objects = obs_space.shape[2]
        self._num_actions = num_outputs

        self.network = nn.Sequential(
            layer_init(nn.Conv2d(self._num_objects, 32, 3, padding=1)),
            nn.ReLU(),
            layer_init(nn.Conv2d(32, 64, 3, padding=1)),
            nn.ReLU(),
            layer_init(nn.Conv2d(64, 64, 3, padding=1)),
            nn.ReLU(),
            layer_init(nn.Conv2d(64, 64, 3, padding=1)),
            nn.ReLU(),
            GlobalAvePool(2048),
            layer_init(nn.Linear(2048, 1024)),
            nn.ReLU(),
            layer_init(nn.Linear(1024, 512)),
            nn.ReLU(),
            layer_init(nn.Linear(512, 512))
        )

        self._actor_head = nn.Sequential(
            layer_init(nn.Linear(512, 512), std=0.01),
            nn.ReLU(),
            layer_init(nn.Linear(512, self._num_actions), std=0.01)
        )

        self._critic_head = nn.Sequential(
            layer_init(nn.Linear(512, 1), std=0.01)
        )

    def forward(self, input_dict, state, seq_lens):
        obs_transformed = input_dict['obs'].permute(0, 3, 1, 2)
        network_output = self.network(obs_transformed)
        value = self._critic_head(network_output)
        self._value = value.reshape(-1)
        logits = self._actor_head(network_output)
        return logits, state

    def value_function(self):
        return self._value


.. seealso:: You can read more about agents that use Global Average Pooling here: https://arxiv.org/abs/2005.11247


****************
Recording Videos
****************

Videos are recorded of the global observer