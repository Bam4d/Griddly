import gymnasium as gym
from typing import Union, Optional
from griddly.gym import GymWrapper

from griddly.typing import ObservationSpace


class RenderWrapper(gym.Wrapper):
    def __init__(
        self, env: GymWrapper, observer: Union[str, int] = 0, render_mode: str = "human"
    ) -> None:
        """
        Used to wrap an environment with an observer.

        Griddly environments typically support multiple observers for multiple players.
        This wrapper lets the user choose which observer to render.


        Args:
            env (gym.Env): The environment to wrap
            observer (Union[str, int], optional): if observer is set to "global" the environment's configured global observer will be rendered. Otherwise the value is the id of the player observer to render. Defaults to 0.

        Examples:

        >>> env_player_0 = RenderWrapper(env, observer=0, "human")
        >>> env_player_1 = RenderWrapper(env, observer=1, "rgb_array")
        >>> env_global = RenderWrapper(env, observer="global")

        >>> env_player_0.render() # Renders player 0's view in a window
        >>> env_player_1.render() # Renders player 1's view in a numpy array
        >>> env_global.render() # Renders the global view in a window


        """

        super().__init__(env)
        self._observer = observer
        self._render_mode = render_mode

        assert isinstance(self.env, GymWrapper)

        
        if observer == "global":
            self.observation_space = env.global_observation_space
        elif isinstance(observer, int):
            if isinstance(env.player_observation_space, list):
                self.observation_space = env.player_observation_space[observer]
            else:
                self.observation_space = env.player_observation_space
        else:
            raise ValueError(f"Observer must be either 'global' or an integer, got {observer}")

    def render(self) -> Union[str, npt.NDArray]: # type: ignore
        assert isinstance(self.env, GymWrapper)
        return self.env.render_observer(self._observer, self._render_mode)

    @property
    def render_mode(self) -> Optional[str]: # type: ignore
        if self._render_mode is None:
            return self.env.render_mode
        return self._render_mode

    @property
    def observer(self) -> Union[int, str]:
        return self._observer