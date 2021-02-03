import numpy as np
from ray.rllib.models.torch.torch_modelv2 import TorchModelV2
from torch import nn


def layer_init(layer, std=np.sqrt(2), bias_const=0.0):
    nn.init.orthogonal_(layer.weight, std)
    nn.init.constant_(layer.bias, bias_const)
    return layer


class GlobalAvePool(nn.Module):

    def __init__(self, final_channels):
        super().__init__()
        self._final_channels = final_channels
        self._pool = nn.Sequential(
            nn.AdaptiveAvgPool3d((final_channels, 1, 1)),
            nn.Flatten(),
        )

    def forward(self, input):
        return self._pool(input)


class GAPAgent(TorchModelV2, nn.Module):
    """
    Global Average Pooling Agent
    This is the same agent used in https://arxiv.org/abs/2011.06363.

    Global average pooling is a simple way to allow training grid-world environments regardless o the size of the grid.
    """

    def __init__(self, obs_space, action_space, num_outputs, model_config, name):
        super().__init__(obs_space, action_space, num_outputs, model_config, name)
        nn.Module.__init__(self)

        self._num_objects = obs_space.original_space['obs'].shape[2]
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
            layer_init(nn.Linear(2048, 512)),
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
        obs_transformed = input_dict['obs']['obs'].permute(0, 3, 1, 2)
        network_output = self.network(obs_transformed)
        value = self._critic_head(network_output)
        self._value = value.reshape(-1)
        logits = self._actor_head(network_output)
        return logits, state

    def value_function(self):
        return self._value
