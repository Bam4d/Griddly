import gym
from collections import Counter
from griddly import GymWrapperFactory, gd
from griddly.util.wrappers import ValidActionSpaceWrapper

class EventFrequencyTracker():

    def __init__(self, window_size):
        self._steps = 0

        self._window_size = window_size

        self._frequency_trackers = [Counter() for _ in range(window_size)]

    def process(self, events):
        for e in events:
            action_name = e['ActionName']
            self._frequency_trackers[-1][action_name] += 1

        self._frequency_trackers.pop(0)
        self._frequency_trackers.append(Counter())

    def get_frequencies(self):
        event_totals = Counter()
        for tracker in self._frequency_trackers:
            for key, value in tracker.items():
                event_totals[key] += value

        event_averages = {}
        for k,v in event_totals.items():
            event_averages[k] = v/self._window_size

        return event_totals


if __name__ == '__main__':
    wrapper = GymWrapperFactory()

    wrapper.build_gym_from_yaml("GriddlyRTS-Adv",
                                'griddly_rts.yaml',
                                global_observer_type=gd.ObserverType.VECTOR,
                                player_observer_type=gd.ObserverType.ISOMETRIC,
                                level=0)

    env_original = gym.make(f'GDY-GriddlyRTS-Adv-v0')
    # env_original = gym.make(f'GDY-GriddlyRTS-Adv-v0')

    env_original.reset()
    env_original.enable_history()

    env = ValidActionSpaceWrapper(env_original)

    event_tracker = EventFrequencyTracker(10)

    for i in range(10000):
        action = env.action_space.sample()

        obs, reward, done, info = env.step(action)

        event_tracker.process(info['History'])

        env.render()

        print(event_tracker.get_frequencies())