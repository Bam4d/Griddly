.. _doc_tutorials_custom_shaders:

##############
Custom Shaders
##############

GDY provides an easy way of scripting most game mechanics. But what if you want to do something interesting with the visualization of the environment?

Griddly achieves high-speed rendering using hardware accelerated `SPIR-V shaders <https://www.khronos.org/opengl/wiki/SPIR-V>`_.

Shaders are complicated beasts, but the following tutorials will help to understand how Griddly uses them. We will also show how you can customize them to produce much more visually complex environments.

This tutorial is not intended to teach how computer graphics pipelines work, you will probably require a basic grasp of several topics. I've included some good resources here that should be helpful:

`Model View Projection Matrices <https://www.youtube.com/watch?v=-tonZsbHty8&ab_channel=JamieKing>`_

`Shaders Basics <https://www.youtube.com/watch?v=kfM-yu0iQBk&ab_channel=FreyaHolm%C3%A9r>`_ (this is based around the Unity engine, but many of the concepts are similar.

`Healthbars, Signed Distance Fields & Lighting <https://www.youtube.com/watch?v=mL8U8tIiRRg>`_ (again around Unity, but we use Signed Distance Fields in Example 2 to make a health bar!

*********************************
Griddly Graphics Pipeline Basics
*********************************

Griddly's rendering pipeline uses a single `Vertex Shader` and a single `Fragment Shader`. The entire game state (global variables, object locations, object variable values etc.) is sent to both the vertex and fragment shader. This allows the shader to read the states and render the environment accordingly.

Vertex Shader
==============


Fragment Shader
================



****************************
Example 1 - Global Lighting
****************************

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted height="10%">

          <source src="../../../../_static/video/tutorials/custom_shaders/global_lighting/global_video_test.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

In this tutorial we use the global variable ``_steps`` in the fragment shader to change the lighting level of the entire environment.

:ref:`Go to tutorial <doc_tutorials_custom_shaders_global_lighting>`


`Go to code <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Custom%20Shaders/Global%20Lighting>`_

.. _example_health_bars:

************************
Example 2 - Health Bars
************************

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted height="10%">

          <source src="../../_static/video/tutorials/custom_shaders/health_bars/global_video_test.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

In this tutorial we use the ``health`` and ``max_health`` variables that we define in the GDY for each object to create a "health bar" showing the health of the agents as they battle. The heath bars are created in the fragment shader using signed distance fields.

:ref:`Go to tutorial <doc_tutorials_custom_shaders_health_bars>`


`Go to code <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Custom%20Shaders/Health%20Bars>`_

****************************
Example 3 - Object Lighting
****************************

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted height="10%">

          <source src="../../_static/video/tutorials/custom_shaders/object_lighting/global_video_test.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

In this tutorial we use the ``is_light`` variables that we define in the GDY to create lights around certain objects. Again we used signed distance fields in the fragment shader to create the light effects.

:ref:`Go to tutorial <doc_tutorials_custom_shaders_object_lighting>`


`Go to code <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Custom%20Shaders/Object%20Lighting>`_