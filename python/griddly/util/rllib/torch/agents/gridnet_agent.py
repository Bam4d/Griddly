from ray.rllib.models.torch.torch_modelv2 import TorchModelV2
from torch import nn
import numpy as np

from griddly.util.rllib.torch.agents.global_average_pooling_agent import layer_init, GlobalAvePool


def deconv_size(i, k, d, s, p, op):
    return (i - 1) * s - 2 * p + d * (k - 1) + op + 1


def conv_size(i, k, d, s, p):
    return np.floor(1 + ((i + 2 * p - d * (k - 1) - 1) / s))


class GridnetAgent(TorchModelV2, nn.Module):
    def __init__(self, obs_space, action_space, num_outputs, model_config, name):
        super().__init__(obs_space, action_space, num_outputs, model_config, name)
        nn.Module.__init__(self)

        height = obs_space.shape[0]
        width = obs_space.shape[1]
        obs_channels = obs_space.shape[2]

        grid_channels = np.sum(action_space.nvec[:int(action_space.shape[0] / (height * width))])

        self._encoder = nn.Sequential(
            nn.Conv2d(obs_channels, 32, kernel_size=3, padding=1),
            nn.MaxPool2d(3, stride=1, padding=1),
            nn.ReLU(),
            nn.Conv2d(32, 64, kernel_size=3, padding=1),
            nn.MaxPool2d(3, stride=1, padding=1),
            nn.ReLU(),
            # nn.Conv2d(64, 128, kernel_size=3, padding=1),
            # nn.MaxPool2d(3, stride=1, padding=1),
            # nn.ReLU(),
            # nn.Conv2d(128, 256, kernel_size=3, padding=1),
            # nn.MaxPool2d(3, stride=1, padding=1),
        )

        self._decode = nn.Sequential(
            # nn.ConvTranspose2d(256, 128, 3, stride=1, padding=1),
            # nn.ReLU(),
            # nn.ConvTranspose2d(128, 64, 3, stride=1, padding=1),
            # nn.ReLU(),
            nn.ConvTranspose2d(64, 32, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.ConvTranspose2d(32, grid_channels, 3, stride=1, padding=1),
            nn.ReLU(),
            nn.Flatten(),
        )

        self.critic = nn.Sequential(
            GlobalAvePool(256),
            nn.Flatten(),
            layer_init(nn.Linear(256, 256)),
            nn.ReLU(),
            layer_init(nn.Linear(256, 1), std=1)
        )

    def forward(self, input_dict, state, seq_lens):
        input_obs = input_dict['obs'].permute(0, 3, 1, 2)
        self._encoded = self._encoder(input_obs)

        # Value function
        value = self.critic(self._encoded)
        self._value = value.reshape(-1)

        # Logits for actions
        logits = self._decode(self._encoded)

        return logits, state

    def value_function(self):
        return self._value
