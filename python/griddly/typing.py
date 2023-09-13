from typing import Union, Dict, Any, List

import numpy.typing as npt
from gymnasium.spaces import Space

Observation = Union[Dict[str, Any], npt.NDArray]
Action = Union[int, List[int], npt.NDArray]

ObservationSpace = Space[Observation]
ActionSpace = Space[Action]
