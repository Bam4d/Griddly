from typing import Union, Dict, Any, List

import numpy.typing as npt

Observation = Union[Dict[str, Any], npt.NDArray]
Action = Union[List[int], npt.NDArray]