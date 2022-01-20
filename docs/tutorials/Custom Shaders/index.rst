.. _doc_tutorials_custom_shaders:

##############
Custom Shaders
##############

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted height="10%">

          <source src="../../_static/video/tutorials/custom_shaders/object_lighting/global_video_test.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text">Partial Observability using a custom lighting shader. See Example 3.</span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

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

Griddly has default shaders for both ``SPRITE_2D`` and ``BLOCK_2D`` renderers which are automatically included when you install Griddly. The code for those shaders (specifically for when you use the ``SPRITE_2D``) is shown below:

Shader Memory Layout
====================

The default vertex shader is slightly more complicated than the fragment shader, as we include all of the possible ``uniform buffer objects`` and ``storage buffer objects`` that are accessible from the shader.

.. note:: All uniforms are available to the vertex and fragment shader.

We use a **Push Constants** to index the object in the ``ObjectData`` that we are currently rendering on the GPU. Each ObjectData contains all the information of a single object as defined in the GDY, such as ``color``, ``textureIdx``, ``objectType`` and position (``modelMatrix``). We also have a persistent ``EnvironmentData`` object which includes global information about the environment, including the ``playerId`` that the shader is constructing the observation for. We also have access to information about the other players in the ``PlayerInfo`` buffer. (Currently this is limited to the automatically generated ``playerColor``).

Finally we have the ``ObjectVariableBuffer`` and ``GlobalVariableBuffer`` which contain the data for **object variables** and **global variables** (as defined in the GDY files under the :ref:`Shader options <#/properties/Environment/properties/Observers/properties/Sprite2D/properties/Shader>`). 

The full shader layout is shown below.:


.. code:: glsl

  struct GlobalVariable {
    int value;
  };

  struct PlayerInfo {
    vec4 playerColor;
  };

  struct ObjectData {
    mat4 modelMatrix;
    vec4 color;
    vec2 textureMultiply;
    int textureIndex;
    int objectType;
    int playerId;
    int zIdx;
  };

  layout(std140, binding = 1) uniform EnvironmentData {
    mat4 projectionMatrix;
    mat4 viewMatrix;
    vec2 gridDims;
    int playerId;
    int globalVariableCount;
    int objectVariableCount;
    int highlightPlayers;
  }
  environmentData;

  layout(std430, binding = 2) readonly buffer PlayerInfoBuffer {
    PlayerInfo variables[];
  }
  playerInfoBuffer;

  layout(std430, binding = 3) readonly buffer ObjectDataBuffer {
    uint size;
    ObjectData variables[];
  }
  objectDataBuffer;

  layout(std430, binding = 4) readonly buffer GlobalVariableBuffer {
    GlobalVariable variables[];
  }
  globalVariableBuffer;

  layout(std430, binding = 5) readonly buffer ObjectVariableBuffer {
    ObjectVariable variables[];
  }
  objectVariableBuffer;

  layout(push_constant) uniform PushConsts {
    int idx;
  }
  pushConsts;

Default Vertex Shader
=====================

The vertex shader is mostly very simple, it takes the object defined by the current ``pushConsts.idx`` and applies the `model view projection` matrix of that object to each vertex. 

The sprite image for the object is stored in a `texture array` indexed by ``object.textureIndex``. This is used to generate the fragment coordinates that are sent through to the fragment shader in the ``outFragTextureCoords`` variable.

Additionally, in the default vertex shader, we allow players to be highlighted by color. This can be seen in many of the multi-agent and RTS games. The player color is stored in the ``outPlayerColor`` variable, which is also sent to the fragment shader.

.. code:: glsl

  void main() {
    ObjectData object = objectDataBuffer.variables[pushConsts.idx];
    PlayerInfo objectPlayerInfo = playerInfoBuffer.variables[object.playerId - 1];

    outFragTextureCoords = vec3(
        inFragTextureCoords.x * object.textureMultiply.x,
        inFragTextureCoords.y * object.textureMultiply.y,
        object.textureIndex);

    mat4 mvp = environmentData.projectionMatrix * environmentData.viewMatrix * object.modelMatrix;

    gl_Position = mvp * vec4(
                            inPosition.x,
                            inPosition.y,
                            inPosition.z,
                            1.);

    if (environmentData.highlightPlayers == 1) {
      if (object.playerId > 0 && object.playerId == environmentData.playerId) {
        outPlayerColor = vec4(0.0, 1.0, 0.0, 1.0);
      } else {
        outPlayerColor = objectPlayerInfo.playerColor;
      }

      outHighlightPlayers = 1;
    } else {
      outHighlightPlayers = 0;
      outPlayerColor = vec4(0.0);
    }
  }

Default Fragment Shader
=======================

Most customization for visualization in Griddly environments is undertaken in the fragment shader. 

The default fragment shader samples the color of each pixel from the texture, given the texture coordinates from the vertex shader. 

In this default shader we also keep player highlighting code which adds an outline to the sprite image based on it's transparency.

.. code:: glsl

  void main()
  {
      if(highlightPlayers==1){
          // Just multiply by the alpha channel of the object
          vec4 color=texture(samplerArray,inFragTextureCoords);
          
          vec2 tex_dims=vec2(textureSize(samplerArray,0));
          
          vec2 pixel_size=2./tex_dims;
          
          vec4 colorU=texture(samplerArray,vec3(inFragTextureCoords.x,max(pixel_size.y,inFragTextureCoords.y-pixel_size.y),inFragTextureCoords.z));
          vec4 colorD=texture(samplerArray,vec3(inFragTextureCoords.x,min(tex_dims.y,inFragTextureCoords.y+pixel_size.y),inFragTextureCoords.z));
          vec4 colorL=texture(samplerArray,vec3(min(tex_dims.x,inFragTextureCoords.x+pixel_size.x),inFragTextureCoords.y,inFragTextureCoords.z));
          vec4 colorR=texture(samplerArray,vec3(max(0.,inFragTextureCoords.x-pixel_size.x),inFragTextureCoords.y,inFragTextureCoords.z));
          
          outFragColor=color;
          
          float thresh1=.7;
          float thresh2=.4;
          
          if(color.a<=thresh1&&(colorU.a>thresh2||colorD.a>thresh2||colorL.a>thresh2||colorR.a>thresh2)){
              outFragColor=playerColor;
          }
          
      }else{
          outFragColor=texture(samplerArray,inFragTextureCoords);
      }
      
  }


.. _customizing_shaders:

********************
Customising Shaders
********************

In order to customize these shaders, firstly the custom shaders have to be coded and compiled and secondly we have to point Griddly to these compiled shaders.

To point Griddly to a new shader directory, the ``shader_path`` parameter needs to be supplied in the ``gym.make`` or ``GymWrapper`` function:

.. code:: python

  env = GymWrapper('object_lighting.yaml',
                     shader_path='shaders',
                     player_observer_type=gd.ObserverType.SPRITE_2D,
                     global_observer_type=gd.ObserverType.SPRITE_2D,
                     ...
                  )

Compiling shaders with glslc
=============================

There are many shader languages that can be compiled into the SPIR-V format that Griddly is compatible with. In all the examples, and default shaders we use the GLSL shader language and compile it to SPIR-V using ``glslc``

glslc can be obtained by installing the `Vulkan SDK <https://vulkan.lunarg.com/>`_ or by forking the `github repo <https://github.com/google/shaderc>`_ and compiling from scratch.

Once you have the glslc  tool, you can compile the fragment and vertex shaders using the following commands:

.. code:: bash
  
  glslc triangle-textured.frag -o $SHADER_OUTPUT_DIR/triangle-textured.frag.spv
  glslc triangle-textured.vert -o $SHADER_OUTPUT_DIR/triangle-textured.vert.spv

.. note:: For SPRITE_2D and ISOMETRIC shaders, the compiled fragment and vertex shaders must be named ``triangle-textured.frag.spv`` and ``triangle-textured.vert.spv``. For BLOCK_2D, the shaders need to be named ``triangle.frag.spv`` and ``triangle.vert.spv``.

*********
Examples
*********

.. toctree::
   :hidden:

   Global Lighting/index
   Health Bars/index
   Object Lighting/index

.. list-table::
   :class: game-gallery

   *  -  **Global Lighting**

         .. raw:: html

            <div class="figure align-center" id="vid1">
                <video onloadeddata="this.play();" playsinline loop muted height="10%">

                    <source src="../../_static/video/tutorials/custom_shaders/global_lighting/global_video_test.mp4"
                            type="video/mp4">

                    Sorry, your browser doesn't support embedded videos.
                </video>
                <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
            </div>

         In this tutorial we use the global variable ``_steps`` in the fragment shader to change the lighting level of the entire environment.

         .. list-table::
             
            *  -  :ref:`Go to tutorial <doc_tutorials_custom_shaders_global_lighting>`
               -  `Go to code <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Custom%20Shaders/Global%20Lighting>`_

   *  -  **Heath Bars**

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

         .. list-table::
             
            *  -  :ref:`Go to tutorial <doc_tutorials_custom_shaders_health_bars>`
               -  `Go to code <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Custom%20Shaders/Health%20Bars>`_
         

   *  -  **Object Lighting**

         .. raw:: html

            <div class="figure align-center" id="vid1">
                <video onloadeddata="this.play();" playsinline loop muted height="10%">

                    <source src="../../_static/video/tutorials/custom_shaders/object_lighting/global_video_test.mp4"
                            type="video/mp4">

                    Sorry, your browser doesn't support embedded videos.
                </video>
                <p class="caption"><span class="caption-text"></span><a class="headerlink" href="#vid1">¶</a></p>
            </div>

         In this tutorial we use variables that we define in the GDY to create lights around certain objects. Again we used signed distance fields in the fragment shader to create the light effects.
         
         .. list-table::
             
            *  -  :ref:`Go to tutorial <doc_tutorials_custom_shaders_object_lighting>`
               -  `Go to code <https://github.com/Bam4d/Griddly/tree/develop/python/examples/Custom%20Shaders/Object%20Lighting>`_