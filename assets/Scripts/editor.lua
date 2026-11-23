-- FOWE editor bootstrap script (Lua 5.3 + sol2)
fowe.log("editor.lua loaded")

fowe.on("editor_ready", function()
    fowe.log("editor ready; event bus online")
end)

fowe.on("quality_changed", function(tier)
    fowe.log("graphics quality changed -> " .. tostring(tier))
end)

fowe.on("physics_settled", function(height)
    fowe.log("dynamic box settled at y = " .. tostring(height))
end)
