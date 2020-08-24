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
	render_window.scene = image!(view(img, :, size(img)[2]:-1:1))
    display(render_window.scene)
    # if you want to see more than the last state you need to sleep for a few
    sleep(1e-4)
    delete!(render_window.scene,render_window.scene[end])
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

# This function will return the stream which we will then be able to add frame
function start(video::VideoRecorder)
	display(video.scene)
	return VideoStream(video.scene;framerate=video.fps)
end

# Add the observation as a frame for our video
function add_frame!(video::VideoRecorder,io::VideoStream,observation;nice_render=false)
	# observation is a 3d array with UInt8, we need to transform it into a rgb julia image
	img = ImageCore.colorview(RGB{N0f8},observation)
	video.scene = image!(view(img, :, size(img)[2]:-1:1))
	# display(video.scene)
	# if (nice_render)
	#     # if you want to see more than the last state you need to sleep for a few
	#     sleep(1e-4)
	# end
	recordframe!(io)
end

# Save the video file_name
function save(video::VideoRecorder,io::VideoStream)
	Makie.save("$(video.saving_path)$(video.file_name)$(video.format)",io)
end
