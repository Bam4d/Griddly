module Griddly

    using CxxWrap
    @wrapmodule(joinpath(@__DIR__,"..","Debug","bin","JuGriddly"),:define_module_jugriddly)

    function __init__()
   		@initcxx
    end

end