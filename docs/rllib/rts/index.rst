.. _doc_rllib_rts:

##################
Real Time Strategy
##################

Griddly also supports Strategy games! Strategy games in the context of Griddly are games where the player can control multiple "units" at at a single time. 

RTS environments similar to multi-agent environments, but the units are controlled by individually selecting them and then performing actions. In this example, only a single action can be send to a particular unit on each turn.


************************
Conditional Action Trees
************************

This example uses Conditional Action Trees in order to handle multiple units and invalid action masking. Conditional Action Trees are implemented in RLLib in the ``ConditionalActionImpalaTrainer``.

.. seealso:: For more information on Conditional Action Trees, see the github repository: https://github.com/Bam4d/conditional-action-trees and the paper: https://arxiv.org/abs/2104.07294.

************
Full Example
************

.. raw:: html

    <div class="figure align-center" id="vid1">
        <video onloadeddata="this.play();" playsinline loop muted width="50%">

            <source src="/_static/video/griddly_rts.mp4"
                    type="video/mp4">

            Sorry, your browser doesn't support embedded videos.
        </video>
        <p class="caption"><span class="caption-text">A video taken during self-training of two agents in the "Griddly-RTS" environment.</span><a class="headerlink" href="#vid1">¶</a></p>
    </div>

In this example, self-play is used to train the agent controlling both armies. We are using a single IMPALA-CNN policy to teach both agents. This means the policy must learn to play from both perspectives (starting from the bottom of the map or the top).

.. code-block:: python

    import os
    import sys

    import ray
    from ray import tune
    from ray.rllib.models import ModelCatalog
    from ray.tune.registry import register_env
    from rts.models import ImpalaCNNAgent

    from griddly import gd
    from griddly.util.rllib.callbacks import MultiCallback, ActionTrackerCallback
    from griddly.util.rllib.environment.core import RLlibMultiAgentWrapper, RLlibEnv
    from griddly.util.rllib.torch.conditional_actions.conditional_action_policy_trainer import \
        ConditionalActionImpalaTrainer

    if __name__ == '__main__':
        sep = os.pathsep
        os.environ['PYTHONPATH'] = sep.join(sys.path)

        ray.init(num_gpus=1)

        env_name = "griddly-rts-env"


        def _create_env(env_config):
            env = RLlibEnv(env_config)
            return RLlibMultiAgentWrapper(env, env_config)


        register_env(env_name, _create_env)
        ModelCatalog.register_custom_model("ImpalaCNN", ImpalaCNNAgent)

        max_training_steps = 100000000

        config = {
            'framework': 'torch',
            '
            'num_workers': 8,
            'num_envs_per_worker': 5,

            'callbacks': ActionTrackerCallback,

            'model': {
                'custom_model': 'ImpalaCNN',
                'custom_model_config': {}
            },
            'env': env_name,
            'env_config': {
                'generate_valid_action_trees': True,
                'yaml_file': 'RTS/Griddly_RTS.yaml',
                'global_observer_type': gd.ObserverType.ISOMETRIC,
                'level': 0,
                'record_actions': True,
                'max_steps': 1000,
            },

            'record_video_config': {
                'frequency': 20000,  # number of rollouts
                'directory': 'videos'
            },

            'entropy_coeff_schedule': [
                [0, 0.001],
                [max_training_steps, 0.0]
            ],
            'lr_schedule': [
                [0, args.lr],
                [max_training_steps, 0.0]
            ],

        }

        stop = {
            "timesteps_total": max_training_steps,
        }

        result = tune.run(ConditionalActionImpalaTrainer, config=config, stop=stop)

