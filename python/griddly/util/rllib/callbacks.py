from typing import Optional, Dict
from collections import Counter
from ray.rllib import Policy, BaseEnv, SampleBatch
from ray.rllib.agents.callbacks import DefaultCallbacks
from ray.rllib.env.base_env import _VectorEnvToBaseEnv
from ray.rllib.evaluation import MultiAgentEpisode
from ray.rllib.utils.typing import PolicyID, AgentID
from wandb import Video


class MultiCallback(DefaultCallbacks):

    def __init__(self, callback_class_list):
        super().__init__()
        self._callback_class_list = callback_class_list

        self._callback_list = []

    def __call__(self, *args, **kwargs):
        self._callback_list = [callback_class() for callback_class in self._callback_class_list]

        return self

    def on_episode_start(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                         episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:
        for callback in self._callback_list:
            callback.on_episode_start(worker=worker, base_env=base_env, policies=policies, episode=episode,
                                      env_index=env_index, **kwargs)

    def on_episode_step(self, *, worker: "RolloutWorker", base_env: BaseEnv, episode: MultiAgentEpisode,
                        env_index: Optional[int] = None, **kwargs) -> None:
        for callback in self._callback_list:
            callback.on_episode_step(worker=worker, base_env=base_env, episode=episode, env_index=env_index, **kwargs)

    def on_episode_end(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                       episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:
        for callback in self._callback_list:
            callback.on_episode_end(worker=worker, base_env=base_env, policies=policies, episode=episode,
                                    env_index=env_index, **kwargs)

    def on_postprocess_trajectory(self, *, worker: "RolloutWorker", episode: MultiAgentEpisode, agent_id: AgentID,
                                  policy_id: PolicyID, policies: Dict[PolicyID, Policy],
                                  postprocessed_batch: SampleBatch, original_batches: Dict[AgentID, SampleBatch],
                                  **kwargs) -> None:
        for callback in self._callback_list:
            callback.on_postprocess_trajectory(worker=worker, episode=episode, agent_id=agent_id, policy_id=policy_id,
                                               policies=policies, postprocessed_batch=postprocessed_batch,
                                               original_batches=original_batches, **kwargs)

    def on_sample_end(self, *, worker: "RolloutWorker", samples: SampleBatch, **kwargs) -> None:
        for callback in self._callback_list:
            callback.on_sample_end(worker=worker, samples=samples, **kwargs)

    def on_learn_on_batch(self, *, policy: Policy, train_batch: SampleBatch, result: dict, **kwargs) -> None:
        for callback in self._callback_list:
            callback.on_learn_on_batch(policy=policy, train_batch=train_batch, result=result, **kwargs)

    def on_train_result(self, *, trainer, result: dict, **kwargs) -> None:
        for callback in self._callback_list:
            callback.on_train_result(trainer=trainer, result=result, **kwargs)


class VideoCallback(DefaultCallbacks):

    def _get_envs(self, base_env):
        if isinstance(base_env, _VectorEnvToBaseEnv):
            return base_env.vector_env.envs
        else:
            return base_env.envs

    def on_episode_start(self,
                         *,
                         worker: "RolloutWorker",
                         base_env: BaseEnv,
                         policies: Dict[PolicyID, Policy],
                         episode: MultiAgentEpisode,
                         env_index: Optional[int] = None,
                         **kwargs) -> None:

        envs = self._get_envs(base_env)
        envs[env_index].on_episode_start(worker.worker_index, env_index)

    def on_episode_end(self,
                       *,
                       worker: "RolloutWorker",
                       base_env: BaseEnv,
                       policies: Dict[PolicyID, Policy],
                       episode: MultiAgentEpisode,
                       env_index: Optional[int] = None,
                       **kwargs) -> None:

        envs = self._get_envs(base_env)
        num_players = envs[env_index].player_count

        info = episode.last_info_for(1) if num_players > 1 else episode.last_info_for()
        if 'videos' in info:
            for video in info['videos']:
                level = video['level']
                path = video['path']
                episode.media[f'level_{level}_1'] = Video(path)


class ActionTrackerCallback(DefaultCallbacks):

    def __init__(self):
        super().__init__()

        self._action_frequency_trackers = {}

    def _get_envs(self, base_env):
        if isinstance(base_env, _VectorEnvToBaseEnv):
            return base_env.vector_env.envs
        else:
            return base_env.envs

    def on_episode_start(self,
                         *,
                         worker: "RolloutWorker",
                         base_env: BaseEnv,
                         policies: Dict[PolicyID, Policy],
                         episode: MultiAgentEpisode,
                         env_index: Optional[int] = None,
                         **kwargs) -> None:
        envs = self._get_envs(base_env)
        num_players = envs[env_index].player_count
        self._action_frequency_trackers[episode.episode_id] = []
        for p in range(0, num_players):
            self._action_frequency_trackers[episode.episode_id].append(Counter())

    def on_episode_step(self,
                        *,
                        worker: "RolloutWorker",
                        base_env: BaseEnv,
                        episode: MultiAgentEpisode,
                        env_index: Optional[int] = None,
                        **kwargs) -> None:

        envs = self._get_envs(base_env)
        num_players = envs[env_index].player_count

        for p in range(0, num_players):
            info = episode.last_info_for(p+1)
            if 'History' in info:
                history = info['History']
                for event in history:
                    action_name = event['ActionName']
                    self._action_frequency_trackers[episode.episode_id][p][action_name] += 1

    def on_episode_end(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                       episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:

        envs = self._get_envs(base_env)
        num_players = envs[env_index].player_count

        for p in range(0, num_players):
            for action_name, frequency in self._action_frequency_trackers[episode.episode_id][p].items():
                episode.custom_metrics[f'agent_info/{p+1}/{action_name}'] = frequency

        del self._action_frequency_trackers[episode.episode_id]
