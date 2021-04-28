from typing import Optional, Dict
from collections import Counter
from ray.rllib import Policy, BaseEnv
from ray.rllib.agents.callbacks import DefaultCallbacks
from ray.rllib.evaluation import MultiAgentEpisode
from ray.rllib.utils.typing import PolicyID
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

        self._action_frequency_trackers = []
        for p in range(1, 3):
            self._action_frequency_trackers.append(Counter())

    def on_episode_step(self,
                        *,
                        worker: "RolloutWorker",
                        base_env: BaseEnv,
                        episode: MultiAgentEpisode,
                        env_index: Optional[int] = None,
                        **kwargs) -> None:

        for p in range(1, 3):
            info = episode.last_info_for(p)
            if 'History' in info:
                history = info['History']
                for event in history:
                    action_name = event['ActionName']
                    self._action_frequency_trackers[p-1][action_name] += 1

    def on_episode_end(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                       episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:
        info = episode.last_info_for(1)
        if 'videos' in info:
            for video in info['videos']:
                level = video['level']
                path = video['path']
                episode.media[f'level_{level}_1'] = Video(path)

        for p in range(1, 3):
            for action_name, frequency in self._action_frequency_trackers[p-1].items():
                episode.custom_metrics[f'agent_info/{p}/{action_name}'] = frequency
