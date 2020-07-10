.. _doc_getting_started_raw:

Raw interface
==============

The raw interface to Griddly exposes the underlying functions that communicate with the native Griddly binary.

The raw interface is much more flexible than the OpenAI gym wrapper, allowing programmatic control of the environment building process.

The following code is a small showcase of the tools that can be used with the Raw interface and how it can be used to control players, and render the player's individual views.

Code
----

.. code-block:: python

    import numpy as np

    from griddly import GriddlyLoader, gd
    from griddly.RenderTools import RenderToFile, RenderWindow, VideoRecorder

    if __name__ == '__main__':
        # A nice tool to save png images
        file_renderer = RenderToFile()

        # Another nice tool for rendering the game state
        player_1_render_window = RenderWindow(500, 500)
        player_2_render_window = RenderWindow(500, 500)
        global_render_window = RenderWindow(500, 500)

        # ANOTHER nice tool for creating videos of the games (requires opencv-python)
        global_video_recorder = VideoRecorder()
        player_1_video_recorder = VideoRecorder()
        player_2_video_recorder = VideoRecorder()

        # The entry point for all raw interfaces
        loader = GriddlyLoader()

        # Load the game description
        grid = loader.load_game_description('RTS/basicRTS.yaml')

        # Load a custom string
        level_string = """  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  M  M  M 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  M  M 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  M 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  H1 .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  W  M  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  B1 .  .  .  .  .  .  .  M  W  M  .  .  .  .  .  .  W  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  .  .  .  .  .  .  M  W  .  .  .  .  .  .  .  W  W  W  W  W  .  .  .  . 
                            .  .  .  .  W  W  W  W  W  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  B2 .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  M  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  M  .  .  .  .  .  .  .  .  H2 .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
                            .  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
                            M  .  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            M  M  .  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  . 
                            M  M  M  .  .  .  .  .  W  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .
        """

        grid.load_level_string(level_string)

        # This defines the global observer
        game = grid.create_game(gd.ObserverType.SPRITE_2D)

        # Defines the player and their observers
        player_1 = game.register_player('Bob', gd.ObserverType.SPRITE_2D)
        player_2 = game.register_player('Alice', gd.ObserverType.SPRITE_2D)

        game.init()
        game.reset()

        global_observation = np.array(game.observe(), copy=False)
        player_1_observation = np.array(player_1.observe(), copy=False)
        player_2_observation = np.array(player_2.observe(), copy=False)

        # Save images of the environment
        file_renderer.render(global_observation, "global_observer.png")
        file_renderer.render(player_1_observation, "player_1_observer.png")
        file_renderer.render(player_2_observation, "player_2_observer.png")

        # Now lets do some random actions
        # First we need to enumerate the actions we can do in the environment
        action_input_mappings = grid.get_action_input_mappings()
        action_definition_count = len(action_input_mappings)

        # These are the actions we can use
        available_action_input_mappings = {}

        action_names = []
        for k, mapping in sorted(action_input_mappings.items()):
            # "Internal" actions are defined in the environment, but cannot be used by any players
            # They can only be spawned from other actions
            if not mapping['Internal']:
                available_action_input_mappings[k] = mapping
                action_names.append(k)

        # We're going to record a video for each of the players observations and the global observer
        player_1_video_recorder.start("player_1_video.mp4", player_1_observation.shape)
        player_2_video_recorder.start("player_2_video.mp4", player_2_observation.shape)
        global_video_recorder.start("global_video.mp4", global_observation.shape)

        for j in range(1000):
            x = np.random.randint(grid.get_width())
            y = np.random.randint(grid.get_height())

            action_definition = np.random.randint(action_definition_count)
            action_name = action_names[action_definition]
            actionId = int(np.random.choice(list(action_input_mappings[action_name]["InputMappings"].keys())))

            # Alternate between player_1 and player_2 actions
            if j % 2 == 0:
                player_1_step_result = player_1.step(action_name, [x, y, actionId])
            else:
                player_2_step_result = player_2.step(action_name, [x, y, actionId])

            # Get the observation states of each of the players
            player_1_observation = np.array(player_1.observe(), copy=False)
            player_2_observation = np.array(player_2.observe(), copy=False)
            global_observation = np.array(game.observe(), copy=False)

            # Render the observation states
            global_render_window.render(global_observation)
            player_1_render_window.render(player_1_observation)
            player_2_render_window.render(player_2_observation)

            # Record the observation states
            player_1_video_recorder.add_frame(player_1_observation)
            player_2_video_recorder.add_frame(player_2_observation)
            global_video_recorder.add_frame(global_observation)

Generating Images - RenderToFile
---------------------------------

The following code is responsible for generating images of the game state:

.. code-block:: python

    global_observation = np.array(game.observe(), copy=False)
    player_1_observation = np.array(player_1.observe(), copy=False)
    player_2_observation = np.array(player_2.observe(), copy=False)

    # Save images of the environment
    file_renderer.render(global_observation, "global_observer.png")
    file_renderer.render(player_1_observation, "player_1_observer.png")
    file_renderer.render(player_2_observation, "player_2_observer.png")

There are three images that are generated. One for each player and one for the "global" observation state

Player 1:

.. thumbnail:: img/player_1_observer.png
   :align: center
   :width: 20%

Player 2:

.. thumbnail:: img/player_2_observer.png
   :align: center
   :width: 20%

Global:

.. thumbnail:: img/global_observer.png
   :align: center
   :width: 20%

Videos - VideoRecorder
----------------------

You can use the following code to add frames to video files of experiments.

.. note:: this requires python-opencv to be installed


Firstly set up the video recorders to write to video files:

.. code-block:: python

    # We're going to record a video for each of the players observations and the global observer
    player_1_video_recorder.start("player_1_video.mp4", player_1_observation.shape)
    player_2_video_recorder.start("player_2_video.mp4", player_2_observation.shape)
    global_video_recorder.start("global_video.mp4", global_observation.shape)
    
Then for each step of the environment you can add the observation states as a video frame:

.. code-block:: python

    # Get the observation states of each of the players
    player_1_observation = np.array(player_1.observe(), copy=False)
    player_2_observation = np.array(player_2.observe(), copy=False)
    global_observation = np.array(game.observe(), copy=False)
    
    # Record the observation states
    player_1_video_recorder.add_frame(player_1_observation)
    player_2_video_recorder.add_frame(player_2_observation)
    global_video_recorder.add_frame(global_observation)


Rendering to Screen - RenderWindow
----------------------------------

Finally its easy to also render your games to the screen:

.. code-block:: python

    # Get the observation states of each of the players
    player_1_observation = np.array(player_1.observe(), copy=False)
    player_2_observation = np.array(player_2.observe(), copy=False)
    global_observation = np.array(game.observe(), copy=False)

    # Render the observation states
    global_render_window.render(global_observation)
    player_1_render_window.render(player_1_observation)
    player_2_render_window.render(player_2_observation)
    


