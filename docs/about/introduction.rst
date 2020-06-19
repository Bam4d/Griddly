.. _doc_about_introduction:

Introduction
============

”Griddly”, an open-source project aimed to be a all-encompassing platform for grid-world based research. Griddly provides an highly optimized game state and rendering engine with a flexible high-level interface for configuring environments. Not only does Griddly offer simple interfaces for single, multi-player and RTS games, but also multiple methods of rendering, configurable partial observability and interfaces for procedural content generation.


One of the most important things about AI research is data. In many Game Environments the rate of data (rendered frames per second, or state representations per second) is relatively slow meaning very long training times. Researchers can compensate for this problem by parallelising the number of games being played, sometimes on expensive hardward and sometimes on several servers requiring network infrastructure to pass states to the actual learning algorithms. For many researchers and hobbyists who want to learn. This approach is unobtainable and only the research teams with lots of funding and engineers supporting the hardware and infrastrcuture required.

The Griddly engine is written entirely in c++ and and uses the `Vulkan API <https://www.khronos.org/vulkan/>`_ to render observational states.

Griddly is written and maintained by Chris Bamford.

Twitter: `@Bam4d <https://twitter.com/Bam4d>`_
Github: `Bam4d <https://github.com/Bam4d>`_



