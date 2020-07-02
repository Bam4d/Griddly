.. _doc_about_introduction:

Introduction
============

One of the most important things about AI research is data. In many Game Environments the rate of data (rendered frames per second, or state representations per second) is relatively slow meaning very long training times. Researchers can compensate for this problem by parallelising the number of games being played, sometimes on expensive hardward and sometimes on several servers requiring network infrastructure to pass states to the actual learning algorithms. For many researchers and hobbyists who want to learn. This approach is unobtainable and only the research teams with lots of funding and engineers supporting the hardware and infrastrcuture required.

Griddly provides a solution to this issue. 

Griddly is an open-source project aimed to be a all-encompassing platform for grid-world based research. Griddly provides a highly optimized game state and rendering engine with a flexible high-level interface for configuring environments. Not only does Griddly offer simple interfaces for single, multi-player and RTS games, but also multiple methods of rendering, configurable partial observability and interfaces for procedural content generation.

Here are some of the highlighted features:

Flexibility
-----------

Griddly games are defined using a simple configuration language GDY in which you can configure the number of players, how inputs are converted into game mechanics, the objects and how they are rendered and what design of the levels.

Read more about :ref:`GDY here<doc_getting_started_gdy>`

Speed + Memory Usage
--------------------

The Griddly engine is written entirely in c++ and and uses the `Vulkan API <https://www.khronos.org/vulkan/>`_ to render observational states. This means that all the games have significantly faster frame rates. Griddly also offers lightweight vectorized state rendering, which can render games states at 30k+ FPS in some games.

Pre-Defined Games
-----------------

Visit the :ref:`games section<doc_games>` here to see which games are currently available. Several games have been ported from the GVGAI and MiniGrid RL environments, which can now be run at significantly higher speeds and less memory overhead.

.. note:: More games are being added as Griddly is being developed. Feel free to design your own games and let the discord community see what your have built!

OpenAI Gym Interface
--------------------

Griddly provides an open ai gym interface out-of-the-box which wraps the underlying raw API making Reinforcement Learning research significantly easier.

Community
=========

Come join the `Griddly Discord <https://discord.gg/HZthby>`_ community, get support and share game levels that you have created.


Griddly is written and maintained by Chris Bamford.

Twitter: `@Bam4d <https://twitter.com/Bam4d>`_
Github: `Bam4d <https://github.com/Bam4d>`_

.. note:: Please help me :D


