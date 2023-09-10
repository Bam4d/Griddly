class LevelGenerator:
    def __init__(self, config: dict) -> None:
        self._config = config

    def generate(self) -> str:
        raise NotImplementedError()
