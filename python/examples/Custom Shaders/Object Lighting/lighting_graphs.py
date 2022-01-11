import matplotlib.pyplot as plt
import numpy as np

def distance(a, b):
    return np.abs(a-b)



def plot_lighting_distance(light_objects, filename):

    x = np.arange(0, 400)
    y = np.zeros(400)
    fig, ax = plt.subplots()

    for lo in light_objects:
        ax.axvline(x=lo, c='r')
        y += 1.0/np.power(distance(lo, x) / 30.0, 2.0) - 0.1

    y = np.maximum(0,np.minimum(1.0, y))


    ax.plot(x, y)
    ax.set_title("Object Light Levels")
    ax.set_xlabel("Distance")
    ax.set_ylabel("Light Level")

    fig.savefig(filename)


def print_lighting_single():
    plot_lighting_distance([100.0], "lighting_single.png")

def print_lighting_multiple():
    plot_lighting_distance([100.0, 300.0], "lighting_multiple.png")


print_lighting_single()
print_lighting_multiple()
