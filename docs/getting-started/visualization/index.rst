.. _doc_visualization:

#############
Visualization
#############

To make it easy for you to create high quality and interesting demonstrations of AIs, or be able to analyse behaviour of trained agents, Griddly provides many tools to make this easy. 


**************
Live Rendering
**************

OpenAI gym allows gym environments to be rendered using the ``env.render()`` function. However in many Griddly environments you may have a choice between different players and global observations to render.

Rendering Different Observers
=============================

To render global observations you simply need to add the parameter ``observer='global'`` to your render function. Additionally if you want to render a particular player you can use ``observer=P`` where P is the 0-indexed player id.

.. code-block:: python
  
  env.render(observer='global') # Will render the global observer

  env.render(observer=0) # Will render the observer for player 1
  
  env.render(observer=1) # Will render the observer for player 2


Player Highlighting
-------------------

In games with multiple players, Griddly will highlight all the objects in the environment based on which observer is being used.

- The global observer will highlight player owned objects with colours which are automatically picked
- The player observer will highlight objects belonging to themselves in green

.. list-table:: Observer Highlighting
   :header-rows: 1

   * - Global
     - Player 1
     - Player 2
   * - .. thumbnail:: img/griddly_rts_global.png
     - .. thumbnail:: img/griddly_rts_p1.png
     - .. thumbnail:: img/griddly_rts_p2.png



*************
Saving Images
*************

Griddly includes a tool ``RenderToFile`` that can directly save observations to disk as png files.

.. code-block:: python

  from griddly.RenderTools import RenderToFile

  render_to_file = RenderToFile()

  ...

  visualization = env.render(observer=.., mode='rgb_array') # Get the observation as an array

  render_to_file.render(visualization, 'my_observation.png') # save the image to disk

*************
Saving Videos
*************

Also saving videos with Griddly is simple using the ``VideoRecorder``.

.. note:: you will need to make sure ffmpeg is installed in your system.

.. code-block:: python

  from griddly.RenderTools import VideoRecorder

  video_recorder = VideoRecorder()

  ...

  # Start the video recording
  observation = env.reset()
  video_recorder.start("video_test.mp4", env.observation_space.shape)

  ...

  # Step the environment and record the next frame
  obs, reward, done, env = env.step( ... )
  video_recorder.add_frame(obs)

  ...
  
  # Clean up
  video_recorder.close()

  

