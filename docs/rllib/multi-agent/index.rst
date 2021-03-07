.. _doc_rllib_multi_agent:

###########
Multi Agent
###########

Griddly automatically wraps multi-agent games for compatibility with RLLib using the `RLLibMultiAgentWrapper`.

To register the multi-agent Griddly environment for usage with RLLib, the environment can be wrapped in the following way:

.. code-block:: python

    # Create the gridnet environment and wrap it in a multi-agent wrapper for self-play
    def _create_env(env_config):
        env = RLlibEnv(env_config)
        return RLlibMultiAgentWrapper(env, env_config)

    register_env(env_name, _create_env)

************
Full Example
************