function get_player_actions_dict(grid)
    dict = Dict{String,Array{Int32,1}}()
    player_actions = convert(Array{String,1},Griddly.get_player_available_actions(grid))
    for action_name in player_actions
        action_input = convert(Array{Int32,1},Griddly.get_input_ids(grid,action_name))
        dict[action_name] = action_input
    end
    return dict
end

function get_map_object_ids_char(grid)
    dict = Dict{Int32,String}()
    list_object_ids = convert(Array{Int32,1},Griddly.get_object_ids_list(grid))
    for object_id in list_object_ids
        object_char = convert(String,Griddly.get_object_char_from_id(grid,object_id))
        dict[object_id] = object_char
    end
    return dict
end

# Need the ObserverType to be ASCII
function pretty_print_ASCII(grid,mapping,observation;sep=" ")
    mapping[0] = "."
    _, width, height = size(observation)
    to_print = []
    for i in 0:(width-1)
        line = ""
        for j in 1:height
            line = string(line,sep,mapping[observation[i*width+j]])
        end
        println(line)
    end
    println("--------------------------")
end
