from typing import Callable, Dict, List, Optional, Tuple, Type, Union

from ray.rllib import SampleBatch, Policy
from ray.rllib.utils import override
from ray.rllib.utils.torch_ops import convert_to_torch_tensor
from ray.rllib.utils.typing import TensorType

import numpy as np
import torch


class InvalidActionMaskingPolicyMixin:

    @override(Policy)
    def compute_actions(
            self,
            obs_batch: Union[List[TensorType], TensorType],
            state_batches: Optional[List[TensorType]] = None,
            prev_action_batch: Union[List[TensorType], TensorType] = None,
            prev_reward_batch: Union[List[TensorType], TensorType] = None,
            info_batch: Optional[Dict[str, list]] = None,
            episodes: Optional[List["MultiAgentEpisode"]] = None,
            explore: Optional[bool] = None,
            timestep: Optional[int] = None,
            **kwargs) -> \
            Tuple[TensorType, List[TensorType], Dict[str, TensorType]]:

        explore = explore if explore is not None else self.config["explore"]
        timestep = timestep if timestep is not None else self.global_timestep

        with torch.no_grad():
            seq_lens = torch.ones(len(obs_batch), dtype=torch.int32)
            input_dict = self._lazy_tensor_dict({
                SampleBatch.CUR_OBS: np.asarray(obs_batch),
                "is_training": False,
            })
            if prev_action_batch is not None:
                input_dict[SampleBatch.PREV_ACTIONS] = \
                    np.asarray(prev_action_batch)
            if prev_reward_batch is not None:
                input_dict[SampleBatch.PREV_REWARDS] = \
                    np.asarray(prev_reward_batch)
            state_batches = [
                convert_to_torch_tensor(s, self.device)
                for s in (state_batches or [])
            ]

            return self._compute_action_helper(input_dict, state_batches,
                                               seq_lens, explore, timestep)

    # @override(Policy)
    # def compute_actions_from_input_dict(
    #         self,
    #         input_dict: Dict[str, TensorType],
    #         explore: bool = None,
    #         timestep: Optional[int] = None,
    #         episodes: Optional[List["MultiAgentEpisode"]] = None,
    #         **kwargs) -> \
    #         Tuple[TensorType, List[TensorType], Dict[str, TensorType]]:
    #     raise NotImplementedError('Invalid action masking cannot be used with Trajectory View API. please set '
    #                               '"_use_trajectory_view_api": False, in the training config.')
