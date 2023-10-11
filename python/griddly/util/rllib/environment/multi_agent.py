from collections import defaultdict
from typing import Any, Dict, List, Optional, Set, Tuple

from ray.rllib.utils.typing import MultiAgentDict

from griddly.typing import Action
from griddly.util.rllib.environment.base import _RLlibEnv
from griddly.util.rllib.environment.observer_episode_recorder import \
    ObserverEpisodeRecorder


class RLlibMultiAgentWrapper(_RLlibEnv):
    def __init__(self, env_config: Dict[str, Any]) -> None:
        super().__init__(env_config)

        self.reset()

        self._player_done_variable = env_config.get("player_done_variable", None)

        # Used to keep track of agents that are active in the environment
        self._active_agents: Set[int] = set()

        assert (
            self._env.player_count > 1
        ), "RLlibMultiAgentWrapper can only be used with environments that have multiple agents"

    def _to_multi_agent_map(self, data: List[Any]) -> MultiAgentDict:
        return {a: data[a - 1] for a in self._active_agents}

    def reset(
        self, seed: Optional[int] = None, options: Optional[Dict[str, Any]] = None
    ) -> Tuple[Dict[int, Any], Dict[Any, Any]]:
        obs, info = self._env.reset(seed, options)
        assert isinstance(obs, list), "RLlibMultiAgentWrapper expects a list of obs"
        self._agent_ids = [a for a in range(1, self._env.player_count + 1)]
        self._active_agents.update([a for a in range(1, self._env.player_count + 1)])
        return self._to_multi_agent_map(obs), info

    def _resolve_player_done_variable(self) -> MultiAgentDict:
        resolved_variables = self._env.game.get_global_variables(
            [self._player_done_variable]
        )
        is_player_done = resolved_variables[self._player_done_variable]
        assert isinstance(
            is_player_done, Dict
        ), "player_done_variable must be a global variable"
        return is_player_done

    def _after_step(
        self,
        obs_map: MultiAgentDict,
        reward_map: MultiAgentDict,
        done_map: MultiAgentDict,
        truncated_map: MultiAgentDict,
        info_map: MultiAgentDict,
    ) -> MultiAgentDict:
        extra_info: MultiAgentDict = {}

        if self.is_video_enabled():
            videos_list = []
            if self.include_agent_videos:
                for a in self._active_agents:
                    assert self._agent_recorders is not None and isinstance(
                        self._agent_recorders, list
                    )
                    end_video = (
                        done_map[a]
                        or done_map["__all__"]
                        or truncated_map[a]
                        or truncated_map["__all__"]
                    )
                    video_info = self._agent_recorders[a].step(
                        self._env.level_id, self.env_steps, end_video
                    )
                    if video_info is not None:
                        videos_list.append(video_info)
            if self.include_global_video and self._global_recorder is not None:
                end_video = done_map["__all__"] or truncated_map["__all__"]
                video_info = self._global_recorder.step(
                    self._env.level_id, self.env_steps, end_video
                )
                if video_info is not None:
                    videos_list.append(video_info)

            self.videos = videos_list

        return extra_info

    def step(
        self, action_dict: MultiAgentDict
    ) -> Tuple[
        MultiAgentDict, MultiAgentDict, MultiAgentDict, MultiAgentDict, MultiAgentDict
    ]:
        actions_array: List[Action] = [] * self._env.player_count
        for agent_id, action in action_dict.items():
            actions_array[agent_id - 1] = action

        obs, reward, all_done, all_truncated, info = self._env.step(actions_array)

        done_map: Dict[str, bool] = {"__all__": all_done}
        truncated_map: Dict[str, bool] = {"__all__": all_truncated}

        if self._player_done_variable is not None:
            griddly_players_done = self._resolve_player_done_variable()

            for agent_id in self._active_agents:
                done_map[agent_id] = griddly_players_done[agent_id] == 1
                truncated_map[
                    agent_id
                ] = False  # TODO: not sure how to support multi-agent truncated?
        else:
            for p in range(1, self._env.player_count + 1):
                done_map[str(p)] = False

        if self.generate_valid_action_trees:
            info_map = self._to_multi_agent_map(
                [
                    {"valid_action_tree": valid_action_tree}
                    for valid_action_tree in info["valid_action_tree"]
                ]
            )
        else:
            info_map = defaultdict(lambda: defaultdict(dict))

        if self.record_actions:
            for event in info["History"]:
                event_player_id = event["PlayerId"]
                if event_player_id != 0:
                    if "History" not in info_map[event_player_id]:
                        info_map[event_player_id]["History"] = []
                    info_map[event_player_id]["History"].append(event)

        assert isinstance(obs, list), "RLlibMultiAgentWrapper expects a list of obs"
        assert isinstance(
            reward, list
        ), "RLlibMultiAgentWrapper expects a list of rewards"
        obs_map = self._to_multi_agent_map(obs)
        reward_map = self._to_multi_agent_map(reward)

        # Finally remove any agent ids that are done
        for agent_id, is_done in done_map.items():
            if is_done:
                self._active_agents.discard(agent_id)

        self._after_step(obs_map, reward_map, done_map, truncated_map, info_map)

        return obs_map, reward_map, done_map, truncated_map, info_map

    def is_video_enabled(self) -> bool:
        return (
            self.record_video_config is not None
            and self._env_idx is not None
            and self._env_idx == 0
        )

    def on_episode_start(self, worker_idx: int, env_idx: int) -> None:
        self._env_idx = env_idx
        self._worker_idx = worker_idx

        if self.is_video_enabled() and not self.video_initialized:
            self.init_video_recording()
            self.video_initialized = True

    def init_video_recording(self) -> None:
        if self.include_agent_videos:
            assert isinstance(self._agent_recorders, list)
            for agent_id in self._agent_ids:
                self._agent_recorders[agent_id] = ObserverEpisodeRecorder(
                    self._env, agent_id - 1, self.video_frequency, self.video_directory
                )
        if self.include_global_video:
            self._global_recorder = ObserverEpisodeRecorder(
                self._env, "global", self.video_frequency, self.video_directory
            )
