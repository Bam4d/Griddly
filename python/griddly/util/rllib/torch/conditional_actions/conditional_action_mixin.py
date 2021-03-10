import numpy as np
import torch
from ray.rllib import Policy, SampleBatch
from ray.rllib.utils import override
from ray.rllib.utils.torch_ops import convert_to_non_torch_type

from griddly.util.rllib.torch.conditional_actions.conditional_action_exploration import TorchConditionalMaskingExploration


class ConditionalActionMixin:

    @override(Policy)
    def compute_actions_from_input_dict(
            self,
            input_dict,
            explore=None,
            timestep = None,
            **kwargs):

        explore = explore if explore is not None else self.config["explore"]
        timestep = timestep if timestep is not None else self.global_timestep

        with torch.no_grad():
            # Pass lazy (torch) tensor dict to Model as `input_dict`.
            input_dict = self._lazy_tensor_dict(input_dict)
            # Pack internal state inputs into (separate) list.
            state_batches = [
                input_dict[k] for k in input_dict.keys() if "state_in" in k[:8]
            ]
            # Calculate RNN sequence lengths.
            seq_lens = np.array([1] * len(input_dict["obs"])) \
                if state_batches else None

            self._is_recurrent = state_batches is not None and state_batches != []

            # Switch to eval mode.
            self.model.eval()

            dist_inputs, state_out = self.model(input_dict, state_batches,
                                                seq_lens)

            infos = input_dict[SampleBatch.INFOS] if SampleBatch.INFOS in input_dict else {}

            valid_action_trees = infos[0]['valid_action_trees'] if isinstance(infos, np.ndarray) and 'valid_action_trees' in infos[0] else None

            exploration = TorchConditionalMaskingExploration(
                self.model,
                dist_inputs,
                valid_action_trees,
                explore,
            )

            actions, masked_logits, logp, mask = exploration.get_actions_and_mask()

            input_dict[SampleBatch.ACTIONS] = actions

            extra_fetches = {
                SampleBatch.ACTION_DIST_INPUTS: dist_inputs,
                SampleBatch.ACTION_PROB: torch.exp(logp.float()),
                SampleBatch.ACTION_LOGP: logp
            }

            # Update our global timestep by the batch size.
            self.global_timestep += len(input_dict[SampleBatch.CUR_OBS])

            return convert_to_non_torch_type((actions, state_out, extra_fetches))
