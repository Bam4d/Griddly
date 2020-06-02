.. _doc_getting_started_installation:

Installation
============
.. warning:: macOS support is :ref:`currently limited<faq_macos>` but will be changing in the future! 

Python
------

Griddly supports versions of python 3.6+.

On most platforms Griddly can be easily installed using:

``pip install griddly``

Virtual environments such as conda are highly recommended to make sure the dependencies of projects using Griddly do not interfere with your other projects.

To create a conda environment with Griddly installed:

.. code-block:: bash

    conda create --name griddly python=3.8
    conda activate griddly
    pip install griddly


Prerequisites
-------------

Griddly uses `Vulkan <https://www.khronos.org/vulkan/>`_ to render environments. Most modern hardware will support vulkan and the required libraries should be pre-installed on your system.

If you are using docker, you can find `images with vulkan <https://hub.docker.com/search?q=vulkan&type=image>`_ pre-installed which may be helpful.


Other Languages
---------------

There is no support currently for languages other than python. A java version may be supported in the near future.

