import numpy as np
import colorsys

class Vector2RGB():

    def __init__(self, scale, observation_channels):
        self._vector_observer_scale = scale

        # Create a colour palette for rendering vector observers
        HSV_tuples = [(x * 1.0 / (observation_channels + 1), 1.0, 1.0) for x in range(observation_channels + 1)]

        vector_rgb = []
        for hsv in HSV_tuples:
            vector_rgb.append(colorsys.hsv_to_rgb(*hsv))

        self._vector_rgb_palette = (np.array(vector_rgb) * 255).astype('uint8')


    def convert(self, observation):
        # add extra dimension so argmax does not get confused by 0 index and empty space
        palette_buffer = np.ones([observation.shape[0] + 1, *observation.shape[1:]]) * 0.5
        palette_buffer[1:] = observation

        # convert to RGB pallette
        vector_pallette = np.argmax(palette_buffer, axis=0).swapaxes(0, 1)

        buffer = self._vector_rgb_palette[vector_pallette]
        # make the observation much bigger by repeating pixels

        return buffer \
            .repeat(self._vector_observer_scale, 0) \
            .repeat(self._vector_observer_scale, 1) \
            .swapaxes(0, 2)