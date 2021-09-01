.. _doc_tutorials_proximity_triggers:

##################
Proximity Triggers
##################

An important mechanic in many environments is to allow events to happen when objects are within certain ranges.

In this example we will create an environment where if the agent (a spider) gets close to lava it may catch fire, and if it goes near water it will douse the flames! Poor spider.

.. raw:: html

  <div class="figure align-center" id="vid1">
      <video onloadeddata="this.play();" playsinline loop muted width="50%">

          <source src="../../_static/video/proximity_triggers.mp4"
                  type="video/mp4">

          Sorry, your browser doesn't support embedded videos.
      </video>
      <p class="caption"><span class="caption-text">An agent "spider" randomly moving around an environment where lava will set the spider on fire and water will douse the flame.</span><a class="headerlink" href="#vid1">¶</a></p>
  </div>

**************************************************
Step 1 - Create the lava, water and spider objects
**************************************************

.. code-block:: YAML
 
  - Name: spider
    Variables:
      - Name: on_fire
        InitialValue: 0
    MapCharacter: s
    Observers:
      Isometric:
        - Image: oryx/oryx_iso_dungeon/avatars/spider-1.png
        - Image: oryx/oryx_iso_dungeon/avatars/spider-fire-1.png
      Block2D:
        - Shape: triangle
          Color: [ 0.2, 0.2, 0.9 ]
          Scale: 0.5
        - Shape: triangle
          Color: [ 0.9, 0.2, 0.2 ]
          Scale: 1.0

  - Name: lava
    MapCharacter: l
    Observers:
      Isometric:
        - Image: oryx/oryx_iso_dungeon/lava-1.png
          Offset: [0, 4]
      Block2D:
        - Color: [ 0.8, 0.0, 0.0 ]
          Shape: square

  - Name: water
    MapCharacter: w
    Observers:
      Isometric:
        - Image: oryx/oryx_iso_dungeon/water-1.png
          Offset: [0, 4]
      Block2D:
        - Color: [ 0.0, 0.0, 0.8 ]
          Shape: square


**********************************************
Step 3 - Set up the proximity trigger for lava
**********************************************

For the lava, we want the spider to be catch fire instantly if it is next to the lava, but have a small chance of catching fire if it is close, but not right next to it.

We can achieve this by using two ``RANGE_BOX_BOUNDARY`` triggers. With ``RANGE_BOX_BOUNDARY`` triggers, only objects that are at a specific distance away can cause the action to trigger.

Therefor using two ``RANGE_BOX_BOUNDARY`` triggers, one with ``Range: 1`` and one with ``Range: 2`` we can produce the desired effect.

Additionally you can set a ``Probability`` for an action to set how likely the action is to be executed.

.. note:: Action Probabilities can be used on any action, not just those with triggers

.. code-block:: YAML

  - Name: set_spider_on_fire_close
    Probability: 1.0
    Trigger:
      Type: RANGE_BOX_BOUNDARY
      Range: 1
    Behaviours:
      - Src:
          Object: lava
        Dst:
          Object: spider
          Commands:
            - set_tile: 1
            - set: [ on_fire, 1 ]


  - Name: set_spider_on_fire
    Probability: 0.1
    Trigger:
      Type: RANGE_BOX_BOUNDARY
      Range: 2
    Behaviours:
      - Src:
          Object: lava
        Dst:
          Object: spider
          Commands:
            - set_tile: 1
            - set: [ on_fire, 1 ]



***********************************************
Step 4 - Set up the proximity trigger for water
***********************************************


The following action uses a ``RANGE_BOX_AREA`` with ```Range: 2`` meaning that anything within a box that is 2 blocks away from the water activates this action

The action has a ``Probability`` of 0.1 of being executed.

.. code-block:: YAML

   - Name: douse_spider
     Probability: 0.1
     Trigger:
       Type: RANGE_BOX_AREA
       Range: 2
     Behaviours:
       - Src:
           Object: water
         Dst:
           Object: spider
           Commands:
             - set_tile: 0
             - set: [ on_fire, 0 ]


********************
Full GDY Description
********************

There's a bit more boiler plate to fill out ... but otherwise proximity triggers are that simple!

.. seealso:: for more information about the boilerplate for GDY files please see :ref:`this tutorial on GDY files<doc_tutorials_gdy>`

.. code-block:: YAML

  Version: "0.1"
  Environment:
    Name: SpiderFire
    Description: Just an example, not a real environment, also not real lava, the spider is real though... too real.
    Observers:
      Block2D:
        TileSize: 24
      Isometric:
        TileSize: [ 32, 48 ]
        IsoTileHeight: 16
        IsoTileDepth: 4
        BackgroundTile: oryx/oryx_iso_dungeon/grass-1.png
      Vector:
        IncludePlayerId: true
    Player:
      AvatarObject: spider
    Levels:
      - |
        w  w  w  w  w  w  .  .  .  .  .  .  .  .  .  .
        w  w  w  w  w  w  .  .  .  .  .  .  .  .  .  .
        w  w  .  .  .  .  .  .  .  .  .  .  .  .  .  .
        w  w  .  .  .  .  .  .  s  .  .  .  .  .  .  .
        w  w  .  .  .  .  .  .  .  .  .  .  .  .  l  l
        w  w  .  .  .  .  .  .  .  .  .  .  .  .  l  l
        .  .  .  .  .  .  .  .  .  .  .  .  l  l  l  l
        .  .  .  .  .  .  .  .  .  .  .  .  l  l  l  l

  Actions:

    - Name: set_spider_on_fire_close
      Probability: 1.0
      Trigger:
        Type: RANGE_BOX_BOUNDARY
        Range: 1
      Behaviours:
        - Src:
            Object: lava
          Dst:
            Object: spider
            Commands:
              - set_tile: 1
              - set: [ on_fire, 1 ]


    - Name: set_spider_on_fire
      Probability: 0.1
      Trigger:
        Type: RANGE_BOX_BOUNDARY
        Range: 2
      Behaviours:
        - Src:
            Object: lava
          Dst:
            Object: spider
            Commands:
              - set_tile: 1
              - set: [ on_fire, 1 ]

    - Name: douse_spider
      Probability: 0.1
      Trigger:
        Type: RANGE_BOX_AREA
        Range: 2
      Behaviours:
        - Src:
            Object: water
          Dst:
            Object: spider
            Commands:
              - set_tile: 0
              - set: [ on_fire, 0 ]


    - Name: move
      Behaviours:
        - Src:
            Object: spider
            Commands:
              - mov: _dest
          Dst:
            Object: _empty

  Objects:
    - Name: spider
      Variables:
        - Name: on_fire
          InitialValue: 0
      MapCharacter: s
      Observers:
        Isometric:
          - Image: oryx/oryx_iso_dungeon/avatars/spider-1.png
          - Image: oryx/oryx_iso_dungeon/avatars/spider-fire-1.png
        Block2D:
          - Shape: triangle
            Color: [ 0.2, 0.2, 0.9 ]
            Scale: 0.5
          - Shape: triangle
            Color: [ 0.9, 0.2, 0.2 ]
            Scale: 1.0

    - Name: lava
      MapCharacter: l
      Observers:
        Isometric:
          - Image: oryx/oryx_iso_dungeon/lava-1.png
            Offset: [0, 4]
        Block2D:
          - Color: [ 0.8, 0.0, 0.0 ]
            Shape: square

    - Name: water
      MapCharacter: w
      Observers:
        Isometric:
          - Image: oryx/oryx_iso_dungeon/water-1.png
            Offset: [0, 4]
        Block2D:
          - Color: [ 0.0, 0.0, 0.8 ]
            Shape: square
