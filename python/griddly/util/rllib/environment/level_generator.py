class LevelGenerator:

    def __init__(self, config):
        self._config = config

    def generate(self):
        raise NotImplementedError()