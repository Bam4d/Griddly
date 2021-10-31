from collections import OrderedDict


class StateHasher():

    def __init__(self, state):
        self._state = state
        self._hash = None

    def hash(self):
        if self._hash is None:
            self._hash = hash(self._state['GameTicks'])
            self._hash ^= hash(frozenset(self._state['GlobalVariables']))
            objects = self._state['Objects']
            sorted_objects = sorted(objects, key=lambda o: (o['Location'][0] * 100 + o['Location'][1], o['Name']))

            ordered_keys_objects = []
            for o in sorted_objects:
                o['Variables'] = frozenset(o['Variables'])
                ordered_keys_objects.append(frozenset(o))

            self._hash ^= hash(frozenset(ordered_keys_objects))

        return self._hash

    def get_state(self):
        return self._state
