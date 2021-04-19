from typing import Optional, Dict

from ray.rllib import Policy, BaseEnv
from ray.rllib.agents.callbacks import DefaultCallbacks
from ray.rllib.evaluation import MultiAgentEpisode
from ray.rllib.utils.typing import PolicyID
from wandb import Video


class GriddlyCallbacks(DefaultCallbacks):

    def on_episode_end(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                       episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:
        pass
            # info = episode.last_info_for()
            # if 'videos' in info:
            #     level = info['video']['level']
            #     path = info['video']['path']
            #     episode.media[f'level_{level}'] = Video(path)
