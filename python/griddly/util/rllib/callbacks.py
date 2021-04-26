from typing import Optional, Dict

from ray.rllib import Policy, BaseEnv, SampleBatch
from ray.rllib.agents.callbacks import DefaultCallbacks
from ray.rllib.evaluation import MultiAgentEpisode
from ray.rllib.utils.typing import PolicyID, AgentID
from wandb import Video


class GriddlyCallbacks(DefaultCallbacks):

    def on_episode_start(self,
                         *,
                         worker: "RolloutWorker",
                         base_env: BaseEnv,
                         policies: Dict[PolicyID, Policy],
                         episode: MultiAgentEpisode,
                         env_index: Optional[int] = None,
                         **kwargs) -> None:
        base_env.envs[env_index].on_episode_start(worker.worker_index, env_index)

    def on_episode_end(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                       episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:
        info = episode.last_info_for(1)
        if 'videos' in info:
            for video in info['videos']:
                level = video['level']
                path = video['path']
                episode.media[f'level_{level}_1'] = Video(path)

