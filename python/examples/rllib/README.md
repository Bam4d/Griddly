# Griddly Rlib Examples

Griddly provides a wrapper for all of its environments to be used with rllib!

We include some examples here of how to use the Griddly `RLLibEnv` environment which wraps any Griddly environment.

## RLLibEnv

All Griddly environments can be used using the `RLLibEnv` custom RLLib environment. 

Firstly, the custom environment needs to be registered using:

```
register_env('your-env-name', RLlibEnv)
```

The environment can then be configured using `env_config` in RLLib's standard config method:

```
'env_config': {

    # Record a video of the environment at the following frequency of steps.
    'record_video_config': {
        'frequency': 100000
    },

    # If all the levels in the environment are the same shape, this will set a random level every time the environment is reset
    'random_level_on_reset': True,
    
    # This is the yaml file of the GDY which defines the game
    'yaml_file': 'Single-Player/GVGAI/clusters_partially_observable.yaml',
    
    # The observer type to use when recording videos
    'global_observer_type': gd.ObserverType.SPRITE_2D,
    
    # The maximum steps of the environment
    'max_steps': 1000
}
```

## Installation

Most of the dependencies you need for these examples are already installed, however we've included a `requirements.txt` for extra things that might be required.

```
cd examples/rllib
pip install -r requirements.txt
```

## Examples

For full documentation please visit: 

### Single-Player
 
The single player example can be modified to use any of the single player games, just change the yaml file and the 
`RLLibEnv` will do the rest. Try it out by running:

`python rllib_single_agent.py`

### Multi-Agent

The multi-agent example plays the game of tag with multiple agents. 

`python rllib_multiagent_taggers.py`

### RTS

Coming soon!