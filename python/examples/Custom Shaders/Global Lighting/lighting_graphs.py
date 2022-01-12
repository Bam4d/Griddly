import matplotlib.pyplot as plt
import numpy as np


def print_day_night_naive():
    x = np.arange(0, 1000)
    y = np.minimum(1, np.cos((np.pi * x) / 360))

    fig, ax = plt.subplots()

    ax.plot(x, y)
    ax.set_title("Day/Night Cycle")
    ax.set_xlabel("_steps")
    ax.set_ylabel("Light Level")

    fig.savefig("daynight_naive.png")

def print_day_night_better():
    x = np.arange(0, 1000)
    y = np.minimum(1, np.cos((np.pi*x)/360)+1)

    fig, ax = plt.subplots()

    ax.plot(x,y)
    ax.set_title("Day/Night Cycle")
    ax.set_xlabel("_steps")
    ax.set_ylabel("Light Level")

    fig.savefig("daynight.png")


print_day_night_naive()
print_day_night_better()
