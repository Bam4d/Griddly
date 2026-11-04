"""asyncio-compatible wrappers around the Griddly nanobind bindings.

All blocking native calls (step, reset, observe, clone, state load/save,
GDY loading) are dispatched to the event loop's default thread-pool
executor via ``loop.run_in_executor``. The C++ bindings release the GIL
while simulating/rendering, so the event loop keeps running (other tasks,
callbacks, awaits) while a step is in flight.

Example:
    import asyncio
    from griddly import AsyncGymWrapper

    async def main():
        env = await AsyncGymWrapper.create(yaml_file="Single-Player/MiniGrid/minigrid_empty.yaml", level=0)
        obs = await env.reset()
        reward, done, info = await env.step(env.action_space.sample())

    asyncio.run(main())
"""

import asyncio
import functools


class AsyncGymWrapper:
    """Async facade over a :class:`griddly.GymWrapper` instance."""

    def __init__(self, env):
        self._env = env

    @classmethod
    async def create(cls, yaml_file=None, yaml_string=None, level=0, gdy=None, **kwargs):
        """Build a GymWrapper in an executor thread and wrap it."""
        from griddly.GymWrapper import GymWrapper

        loop = asyncio.get_running_loop()
        env = await loop.run_in_executor(
            None,
            functools.partial(
                GymWrapper,
                yaml_file=yaml_file,
                yaml_string=yaml_string,
                level=level,
                gdy=gdy,
                **kwargs,
            ),
        )
        return cls(env)

    @property
    def env(self):
        """The underlying synchronous GymWrapper."""
        return self._env

    def __getattr__(self, name):
        # Passthrough for properties (action_space, player_count, ...) and
        # non-blocking attributes of the wrapped env.
        return getattr(self._env, name)

    async def _run(self, fn, *args, **kwargs):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, functools.partial(fn, *args, **kwargs))

    async def step(self, action):
        return await self._run(self._env.step, action)

    async def reset(self, level_id=None, level_string=None, global_observations=False):
        return await self._run(
            self._env.reset,
            level_id=level_id,
            level_string=level_string,
            global_observations=global_observations,
        )

    async def observe(self, player=0):
        """Observe the global game (player=0) or a specific player (1-based)."""
        if player == 0:
            fn = self._env.game.observe
        else:
            fn = self._env._players[player - 1].observe
        return await self._run(fn)

    async def render(self, mode="human", observer=0):
        return await self._run(self._env.render, mode, observer)

    async def get_state(self):
        return await self._run(self._env.get_state)

    async def load_state(self, state):
        loaded = await self._run(self._env.load_state, state)
        return AsyncGymWrapper(loaded)

    async def clone(self):
        cloned = await self._run(self._env.clone)
        return AsyncGymWrapper(cloned)

    async def seed(self, seed=None):
        return await self._run(self._env.seed, seed)

    async def enable_history(self, enable=True):
        return await self._run(self._env.enable_history, enable)

    async def close(self):
        return await self._run(self._env.close)


class AsyncGriddlyLoader:
    """Async facade over :class:`griddly.GriddlyLoader` (GDY parsing)."""

    def __init__(self, loader=None):
        if loader is None:
            from griddly import GriddlyLoader

            loader = GriddlyLoader()
        self._loader = loader

    def __getattr__(self, name):
        return getattr(self._loader, name)

    async def _run(self, fn, *args, **kwargs):
        loop = asyncio.get_running_loop()
        return await loop.run_in_executor(None, functools.partial(fn, *args, **kwargs))

    async def load(self, gdy_path):
        return await self._run(self._loader.load, gdy_path)

    async def load_string(self, yaml_string):
        return await self._run(self._loader.load_string, yaml_string)
