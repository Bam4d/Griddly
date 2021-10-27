from collections import OrderedDict


class StateHasher():

    def __init__(self, state):
        self._state = state
        self._hash = None

    def hash(self):
        if self._hash is None:
            self._hash = hash(self._state['GameTicks'])
            self._hash += hash(repr(OrderedDict(self._state['GlobalVariables'])))
            objects = self._state['Objects']
            self._hash += hash(repr(sorted(objects, key=lambda o: o['Location'][0] * 100 + o['Location'][1])))

        return self._hash

    def get_state(self):
        return self._state
