mutable struct RenderWindow
	scene::SceneLike
	width::Int
	height::Int
	initialized::Bool
end

function RenderWindow(width::Int, height::Int)
	scene = Scene(resolution=(width,height),show_axis=false)
	RenderWindow(scene,width,height,true)
end

function render(render_window::RenderWindow,observation)
	if (!render_window.initialized)
		throw("Render Window is not initialized")
	end

	# observation is a 3d array with UInt8, we need to transform it into a rgb julia image
	img = ImageCore.colorview(RGB{N0f8},observation)
	# add the image to scene
	render_window.scene = image!(view(img, :, size(img)[2]:-1:1))
    display(render_window.scene)
    # if you want to see more than the last state you need to sleep for a few
    sleep(1e-4)
	# clear the stack of plots for memory purpose
    pop!(render_window.scene.plots)
end

function save_frame(observation,resolution::Tuple{Int64,Int64},file_name::String;file_path="julia/img/",format=".png")
	scene = Scene(resolution=resolution,show_axis=false)
	img = ImageCore.colorview(RGB{N0f8},observation)
	scene = image!(view(img, :, size(img)[2]:-1:1))
	Makie.save("$file_path$file_name$format",scene)
end

function save_frame(scene::SceneLike,file_name::String;file_path="julia/img/",format=".png")
	Makie.save("$file_path$file_name$format",scene)
end

mutable struct VideoRecorder
	scene::SceneLike
	fps::Int64
	format::String
	file_name::String
	saving_path::String
end

function VideoRecorder(resolution::Tuple{Int64,Int64},file_name::String; fps=30 ,format=".mp4", saving_path="videos/")
	scene = Scene(resolution=resolution,show_axis=false)
	VideoRecorder(scene,fps,format,file_name,saving_path)
end

function VideoRecorder(scene::SceneLike,file_name::String; fps=30 ,format=".mp4", saving_path="videos/")
	VideoRecorder(scene,fps,format,file_name,saving_path)
end

# This function will return the stream which we will then be able to add frame
function start(video::VideoRecorder)
	return VideoStream(video.scene;framerate=video.fps)
end

# Add the observation as a frame for our video
function add_frame!(video::VideoRecorder,io::VideoStream,observation;nice_render=false)
	# observation is a 3d array with UInt8, we need to transform it into a rgb julia image
	img = ImageCore.colorview(RGB{N0f8},observation)
	# add the img to the scene
	video.scene = image!(view(img, :, size(img)[2]:-1:1))
	display(video.scene)
	if (nice_render)
	    # if you want to see more than the last state you need to sleep for a few
	    sleep(1e-4)
	end
	# add the current frame to io
	recordframe!(io)
	# clear the stack of plots for memory purpose
	pop!(video.scene.plots)
end

# Save the video file_name
function save_video(video::VideoRecorder,io::VideoStream)
	Makie.save("$(video.saving_path)$(video.file_name)$(video.format)",io)
end

struct MultipleScreen
	scene::SceneLike
	width::Int
	height::Int
	nb_scene::Int
	subscenes::Array{SceneLike}
end

function MultipleScreen(width,height;nb_scene=2)
	scene = Scene(resolution=(width,height),show_axis=false)
	# now we have to discretize our screen into nb_scene
	# we first cut our screen along the longer between width and height
	subscenes = []
	if height >= width
		# the height of our cutting scenes will be y_bound
		y_bound = round(height/2)
		if nb_scene%2==0
			# if we got an even number of scene their width will be x_step
			x_step = round(width/(nb_scene/2))
			for j in 0:1
				for i in 0:((nb_scene/2)-1)
					subscene = Scene(scene,Rect(i*x_step,j*y_bound,x_step,y_bound))
					push!(subscenes,subscene)
				end
			end
		else
			# for an odd number of scene we got the upper screen scene's width
			x_step_up = round(width/((nb_scene+1)/2))
			for i in 0:(((nb_scene+1)/2)-1)
				subscene = Scene(scene,Rect(i*x_step_up,y_bound,x_step_up,y_bound))
				push!(subscenes,subscene)
			end
			# and the bottom screen scene's width
			x_step_down = round(width/((nb_scene+1)/2 - 1))
			for i in 0:(((nb_scene+1)/2)-2)
				subscene = Scene(scene,Rect(i*x_step_down,0,x_step_down,y_bound))
				push!(subscenes,subscene)
			end
		end
	else
		# the width of our cutting scenes will all be x_bound
		x_bound = round(width/2)
		if nb_scene%2==0
			# if we got an even number of scene their height will be y_step
			y_step = round(height/(nb_scene/2))
			for j in 0:1
				for i in 0:((nb_scene/2)-1)
					subscene = Scene(scene,Rect(j*x_bound,i*y_step,x_bound,y_step))
					push!(subscenes,subscene)
				end
			end
		else
			# for an odd number of scene we got the right screen scene's height
			y_step_right = round(height/((nb_scene+1)/2 - 1))
			for i in 0:(((nb_scene+1)/2)-2)
				subscene = Scene(scene,Rect(x_bound,i*y_step_right,x_bound,y_step_right))
				push!(subscenes,subscene)
			end
			# and the left screen scene's height
			y_step_left = round(height/((nb_scene+1)/2))
			for i in 0:(((nb_scene+1)/2)-1)
				subscene = Scene(scene,Rect(0,i*y_step_left,x_bound,y_step_left))
				push!(subscenes,subscene)
			end
		end
	end
	return MultipleScreen(scene,width,height,nb_scene,subscenes)
end

function render_multiple(screen::MultipleScreen,observations)
	if screen.nb_scene < length(observations)
		throw("You want to display more observations than you have available scene \n Initialize a MultipleScreen with more Scene")
	end
	for i in 1:length(observations)
		# observation is a 3d array with UInt8, we need to transform it into a rgb julia image
		img = ImageCore.colorview(RGB{N0f8},observations[i])
		image!(screen.subscenes[i],view(img, :, size(img)[2]:-1:1))
	end
    display(screen.scene)
    # if you want to see more than the last state you need to sleep for a few
    sleep(1e-4)
	# empty the stack of the plots for memory purpose
	for i in 1:screen.nb_scene
    	pop!(screen.subscenes[i].plots)
	end
end
