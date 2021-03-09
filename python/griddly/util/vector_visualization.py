import numpy as np
import colorsys


class Vector2RGB():

    def __init__(self, scale, object_channels):
        self._vector_observer_scale = scale

        self._object_channels = object_channels

        # Create a colour palette for rendering vector observers
        HSV_tuples = [(x * 1.0 / (object_channels + 1), 1.0, 1.0) for x in range(object_channels + 1)]

        vector_rgb = []
        for hsv in HSV_tuples:
            vector_rgb.append(colorsys.hsv_to_rgb(*hsv))

        self._vector_rgb_palette = (np.array(vector_rgb) * 255).astype('uint8')

    def convert(self, observation):
        # Add extra dimension so argmax does not get confused by 0 index and empty space
        palette_buffer = np.ones([self._object_channels + 1, *observation.shape[1:]]) * 0.5

        # only used for debugging
        offset = 0

        # Only consider the object type when rendering
        palette_buffer[1:] = observation[offset:self._object_channels+offset, :, :]

        # Convert to RGB pallette
        vector_pallette = np.argmax(palette_buffer, axis=0).swapaxes(0, 1)

        buffer = self._vector_rgb_palette[vector_pallette]

        # Make the observation much bigger by repeating (this is horribly expensive)
        return buffer \
            .repeat(self._vector_observer_scale, 0) \
            .repeat(self._vector_observer_scale, 1) \
            .swapaxes(0, 2)
