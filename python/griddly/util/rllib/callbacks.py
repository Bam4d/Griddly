from typing import Optional, Dict

from ray.rllib import Policy, SampleBatch, BaseEnv
from ray.rllib.agents.callbacks import DefaultCallbacks
from ray.rllib.evaluation import MultiAgentEpisode
from ray.rllib.utils.typing import AgentID, PolicyID
from wandb import Video


class GriddlyCallbacks(DefaultCallbacks):

    def __init__(self, legacy_callbacks_dict: Dict[str, callable] = None):
        super().__init__(legacy_callbacks_dict)

    def on_episode_start(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                         episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:
        super().on_episode_start(worker=worker, base_env=base_env, policies=policies, episode=episode,
                                 env_index=env_index, **kwargs)

    def on_episode_step(self, *, worker: "RolloutWorker", base_env: BaseEnv, episode: MultiAgentEpisode,
                        env_index: Optional[int] = None, **kwargs) -> None:
        super().on_episode_step(worker=worker, base_env=base_env, episode=episode, env_index=env_index, **kwargs)

    def on_episode_end(self, *, worker: "RolloutWorker", base_env: BaseEnv, policies: Dict[PolicyID, Policy],
                       episode: MultiAgentEpisode, env_index: Optional[int] = None, **kwargs) -> None:
        super().on_episode_end(worker=worker, base_env=base_env, policies=policies, episode=episode,
                               env_index=env_index, **kwargs)
        if not worker.multiagent:
            info = episode.last_info_for()
            if 'video' in info:
                level = info['video']['level']
                path = info['video']['path']
                print(f'creating video with path: {path}')
                episode.media['video_test'] = 'here is some test data'
                episode.media[f'level_{level}'] = Video(path)

    def on_postprocess_trajectory(self, *, worker: "RolloutWorker", episode: MultiAgentEpisode, agent_id: AgentID,
                                  policy_id: PolicyID, policies: Dict[PolicyID, Policy],
                                  postprocessed_batch: SampleBatch, original_batches: Dict[AgentID, SampleBatch],
                                  **kwargs) -> None:
        super().on_postprocess_trajectory(worker=worker, episode=episode, agent_id=agent_id, policy_id=policy_id,
                                          policies=policies, postprocessed_batch=postprocessed_batch,
                                          original_batches=original_batches, **kwargs)

    def on_sample_end(self, *, worker: "RolloutWorker", samples: SampleBatch, **kwargs) -> None:
        super().on_sample_end(worker=worker, samples=samples, **kwargs)

    def on_learn_on_batch(self, *, policy: Policy, train_batch: SampleBatch, result: dict, **kwargs) -> None:
        pass
        # Loop through the 'info' keys looking for 'video'
        # for info_dict in train_batch[SampleBatch.INFOS]:
        #     if 'video' in info_dict:
        #         level = info_dict['video']['level']
        #         path = info_dict['video']['path']
        #         print(f'creating video with path: {path}')
        #         result['video_test'] = 1
        #         result[f'level_{level}'] = Video(path)

    def on_train_result(self, *, trainer, result: dict, **kwargs) -> None:
        super().on_train_result(trainer=trainer, result=result, **kwargs)
