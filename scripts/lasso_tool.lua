-- @Title: Lasso Tool
-- @Description: A selection tool that allows you to draw a freehand polygon
-- @Author: Michy
-- @Version: 1.1
-- @Autorun: true

-- State
local active = false
local points = {}
local overlayId = "LassoOverlay"
local startPos = nil
local lastPos = nil

-- Configuration
local OVERLAY_COLOR = {255, 255, 255, 255} -- White
local OVERLAY_WIDTH = 1

-- Register overlay
app.mapView:addOverlay(overlayId, {
    ondraw = function(ctx)
        if not active or #points < 2 then return end

        local cmdPoints = {}
        for _, p in ipairs(points) do
            table.insert(cmdPoints, {x=p.x, y=p.y})
        end

        -- Draw lines (dotted style if supported)
        for i = 1, #cmdPoints - 1 do
            ctx:line{
                x1 = cmdPoints[i].x, y1 = cmdPoints[i].y, z1 = startPos.z,
                x2 = cmdPoints[i+1].x, y2 = cmdPoints[i+1].y, z2 = startPos.z,
                color = OVERLAY_COLOR,
                width = OVERLAY_WIDTH,
                style = "dotted"
            }
        end
    end
})

-- Event Listeners

-- Mouse Press: Start drawing (Ctrl + Alt + Left Click)
app.events:on("onMousePress", function(x, y, z, button)
    if button ~= "left" then return end

    -- Check requirements: Ctrl + Alt must be pressed
    if not (app.keyboard.isCtrlDown() and app.keyboard.isAltDown()) then
        return
    end

    active = true
    startPos = {x=x, y=y, z=z}
    points = {{x=x, y=y}}
    lastPos = {x=x, y=y}

    app.refresh()
    return true -- Consume event
end)

-- Mouse Move: Add points
app.events:on("onMouseMove", function(x, y, z)
    if not active then return end

    -- Only add point if it moved at least 1 tile
    if x ~= lastPos.x or y ~= lastPos.y then
        table.insert(points, {x=x, y=y})
        lastPos = {x=x, y=y}
        app.refresh()
    end

    return true -- Consume event
end)

-- Mouse Release: Finish drawing and select
app.events:on("onMouseRelease", function(x, y, z, button)
    if not active or button ~= "left" then return end

    active = false

    -- Close the loop
    if #points > 2 then
        -- Calculate bounding box
        local minX, maxX, minY, maxY = points[1].x, points[1].x, points[1].y, points[1].y
        for _, p in ipairs(points) do
            minX = math.min(minX, p.x)
            maxX = math.max(maxX, p.x)
            minY = math.min(minY, p.y)
            maxY = math.max(maxY, p.y)
        end

        local sel = app.selection

        -- Start selection session (Internal batch)
        -- Using explicit start/finish for performance if implemented safely in C++ API
        -- If unstable, comment out start/finish to use atomic adds.
        sel:start()

        -- Clear existing if Shift is NOT pressed (Standard behavior: Shift = Add to selection)
        if not app.keyboard.isShiftDown() then
            sel:clear()
        end

        -- Convert points for API
        local polyPoints = {}
        for i, p in ipairs(points) do
            table.insert(polyPoints, {x=p.x, y=p.y})
        end

        -- Find tiles inside polygon
        for cy = minY, maxY do
            for cx = minX, maxX do
                -- Use center of tile for more accurate "inside" check
                if geo.pointInPolygon(cx, cy, polyPoints) then
                    local tile = app.map:getTile(cx, cy, startPos.z)
                    if tile then
                        sel:add(tile)
                    end
                end
            end
        end

        -- Commit selection
        sel:finish()
    end

    points = {}
    app.refresh()
    return true -- Consume event
end)
