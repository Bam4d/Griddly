from ray.rllib.models.torch.torch_modelv2 import TorchModelV2
from torch import nn
import torch

from griddly.util.rllib.torch.agents.global_average_pooling_agent import layer_init


class GridnetAgent(TorchModelV2, nn.Module):
    def __init__(self, obs_space, action_space, num_outputs, model_config, name):
        super().__init__(obs_space, action_space, num_outputs, model_config, name)
        nn.Module.__init__(self)

        height = obs_space.original_space['obs'].shape[0]
        width = obs_space.original_space['obs'].shape[1]

        grid_channels = 78

        self.encoder = nn.Sequential(
            layer_init(nn.Conv2d(27, 32, kernel_size=5, padding=2)),
            nn.ReLU(),
            layer_init(nn.Conv2d(32, 64, kernel_size=5, padding=2)),
            nn.ReLU(),
            layer_init(nn.Conv2d(128, 128, kernel_size=3, padding=1)),
            nn.ReLU()
        )

        self.actor = nn.Sequential(
            layer_init(nn.Conv2d(128, grid_channels, kernel_size=1)),
        )

        self.critic = nn.Sequential(
            nn.Flatten(),
            layer_init(nn.Linear(64 * height * width, 256)),
            nn.ReLU(),
            layer_init(nn.Linear(256, 1), std=1))

    def forward(self, input_dict, state, seq_lens):
        input_obs = input_dict['obs']['obs']
        input_mask = input_dict['obs']['invalid_action_masks']
        self._encoded = self.encoder(input_obs)

        # Value function
        value = self.critic(self._encoded)
        self._value = value.reshape(-1)

        # Logits for actions
        logits = self.actor(self._encoded)

        masked_logits = logits + torch.log(input_mask)

        return masked_logits, state

    def value_function(self):
        return self._value