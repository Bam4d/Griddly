from typing import Optional, Dict
from collections import Counter
from ray.rllib import Policy, BaseEnv
from ray.rllib.algorithms.callbacks import DefaultCallbacks
from ray.rllib.env.vector_env import VectorEnvWrapper
from ray.rllib.evaluation import MultiAgentEpisode
from ray.rllib.utils.typing import PolicyID
from wandb import Video


class GriddlyRLLibCallbacks(DefaultCallbacks):
    """Contains helper functions for Griddly callbacks"""

    def _get_envs(self, base_env):
        if isinstance(base_env, VectorEnvWrapper):
            return base_env.vector_env.get_sub_environments()
        else:
            return base_env.envs

    def _get_player_ids(self, base_env, env_index):
        envs = self._get_envs(base_env)
        player_count = envs[env_index].player_count
        if player_count == 1:
            return ["agent0"]
        else:
            return [p for p in range(1, player_count + 1)]


class VideoCallbacks(GriddlyRLLibCallbacks):
    def on_episode_start(
            self,
            *,
            worker: "RolloutWorker",
            base_env: BaseEnv,
            policies: Dict[PolicyID, Policy],
            episode: MultiAgentEpisode,
            env_index: Optional[int] = None,
            **kwargs,
    ) -> None:
        envs = self._get_envs(base_env)
        envs[env_index].on_episode_start(worker.worker_index, env_index)

    def on_episode_end(
            self,
            *,
            worker: "RolloutWorker",
            base_env: BaseEnv,
            policies: Dict[PolicyID, Policy],
            episode: MultiAgentEpisode,
            env_index: Optional[int] = None,
            **kwargs,
    ) -> None:
        envs = self._get_envs(base_env)

        for video in envs[env_index].videos:
            level = video["level"]
            path = video["path"]
            episode.media[f"level_{level}"] = Video(path)

        envs[env_index].videos = []


class ActionTrackerCallbacks(GriddlyRLLibCallbacks):
    def __init__(self):
        super().__init__()

        self._action_frequency_trackers = {}

    def on_episode_start(
            self,
            *,
            worker: "RolloutWorker",
            base_env: BaseEnv,
            policies: Dict[PolicyID, Policy],
            episode: MultiAgentEpisode,
            env_index: Optional[int] = None,
            **kwargs,
    ) -> None:
        self._action_frequency_trackers[episode.episode_id] = []
        for _ in self._get_player_ids(base_env, env_index):
            self._action_frequency_trackers[episode.episode_id].append(Counter())

    def on_episode_step(
            self,
            *,
            worker: "RolloutWorker",
            base_env: BaseEnv,
            episode: MultiAgentEpisode,
            env_index: Optional[int] = None,
            **kwargs,
    ) -> None:

        for p, id in enumerate(self._get_player_ids(base_env, env_index)):
            info = episode.last_info_for(id)
            if "History" in info:
                history = info["History"]
                for event in history:
                    action_name = event["ActionName"]
                    self._action_frequency_trackers[episode.episode_id][p][
                        action_name
                    ] += 1

    def on_episode_end(
            self,
            *,
            worker: "RolloutWorker",
            base_env: BaseEnv,
            policies: Dict[PolicyID, Policy],
            episode: MultiAgentEpisode,
            env_index: Optional[int] = None,
            **kwargs,
    ) -> None:

        for p, id in enumerate(self._get_player_ids(base_env, env_index)):
            for action_name, frequency in self._action_frequency_trackers[
                episode.episode_id
            ][p].items():
                episode.custom_metrics[f"agent_info/{id}/{action_name}"] = frequency

        del self._action_frequency_trackers[episode.episode_id]


class WinLoseMetricCallbacks(GriddlyRLLibCallbacks):
    def __init__(self):
        super().__init__()

    def on_episode_end(
            self,
            *,
            worker: "RolloutWorker",
            base_env: BaseEnv,
            policies: Dict[PolicyID, Policy],
            episode: MultiAgentEpisode,
            env_index: Optional[int] = None,
            **kwargs,
    ) -> None:
        for p, id in enumerate(self._get_player_ids(base_env, env_index)):
            info = episode.last_info_for(id)
            episode.custom_metrics[f"agent_info/{id}/win"] = (
                1 if info["PlayerResults"][f"{p + 1}"] == "Win" else 0
            )
            episode.custom_metrics[f"agent_info/{id}/lose"] = (
                1 if info["PlayerResults"][f"{p + 1}"] == "Lose" else 0
            )
            episode.custom_metrics[f"agent_info/{id}/end"] = (
                1 if info["PlayerResults"][f"{p + 1}"] == "End" else 0
            )
