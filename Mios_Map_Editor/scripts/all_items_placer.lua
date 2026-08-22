-- Script to place all available items starting at 0,0 with 60 items per row

local function placeAllItems()
    local map = app.map
    if not map then
        app.alert("No map open!")
        return
    end

    local maxPerRow = 60
    local startX = 0
    local startY = 0
    local startZ = 7

    -- Wrap in transaction for single Undo step and performance
    app.transaction("Place All Items", function()
        -- Move camera to start position
        app.setCameraPosition(startX, startY, startZ)

        local currentX = startX
        local currentY = startY
        local maxId = Items.getMaxId()

        -- Iterate through all possible item IDs
        for id = 100, maxId do
            if Items.exists(id) then
                local tile = map:getOrCreateTile(currentX, currentY, startZ)
                if tile then
                    -- Add the item
                    tile:addItem(id)
                end

                -- Update position for next item
                currentX = currentX + 1
                if currentX >= startX + maxPerRow then
                    currentX = startX
                    currentY = currentY + 1
                end
            end
        end
    end)

    app.alert("Placed items up to ID " .. Items.getMaxId())
end

local function undoLastAction()
    local editor = app.editor
    if editor and editor:canUndo() then
        editor:undo()
    else
        app.alert("Nothing to undo!")
    end
end

-- Create UI
local dialog = Dialog("Item Placer")

dialog:label({text = "Places all available items starting at 0,0\n(Rows of 60 items)"})
dialog:separator()



dialog:button({text = "Place All Items", onclick = placeAllItems})
dialog:newrow()
dialog:button({text = "Undo Last Action", onclick = undoLastAction})

-- Check if Overlay API is available
if not app.mapView then
    -- app.alert("Map overlay API not available. Rebuild the editor to enable app.mapView.")
else
    -- Inspector Overlay State
    local inspectorEnabled = false

    -- Function to handle tooltip generation
    local function onHover(info)
        -- We can rely on setEnabled to filter calls, but double check
        if not inspectorEnabled then return nil end
        if not info.tile then return nil end

        local items = info.tile.items
        -- If no items and no ground, nothing to show
        if #items == 0 and not info.tile.ground then return nil end

        local text = ""
        local count = 0

        -- Show Ground Info
        if info.tile.ground then
            local g = info.tile.ground
            text = text .. string.format("Ground: %s (ID: %d)\n", g.name, g.id)
        end

        -- Show Items Info (Top 5)
        -- Items are typically stacked bottom-up, we often want to see top items first.
        -- tile.items usually returns them in stack order (0=bottom).
        -- Let's reverse to show top items first.
        for i = #items, 1, -1 do
            local item = items[i]
            if count < 10 then
                text = text .. string.format("Item: %s (ID: %d)", item.name, item.id)
                if item.count > 1 then
                    text = text .. string.format(" [x%d]", item.count)
                end
                if item.actionId > 0 then
                    text = text .. string.format(" AID:%d", item.actionId)
                end
                if item.uniqueId > 0 then
                    text = text .. string.format(" UID:%d", item.uniqueId)
                end
                text = text .. "\n"
                count = count + 1
            end
        end

        if #items > 10 then
            text = text .. string.format("...and %d more items", #items - 10)
        end

        -- Return table with highlight and tooltip
        return {
            highlight = {
                color = { r = 0, g = 255, b = 255, a = 100 },
                filled = true
            },
            tooltip = {
                text = text,
                color = { r = 255, g = 255, b = 255, a = 255 }
            }
        }
    end

    -- Register the overlay
    app.mapView:addOverlay("item_inspector", {
        enabled = false, -- Start disabled
        order = 500,
        onhover = onHover
    })

    -- Update overlay state helper
    local function updateInspector(enabled)
        inspectorEnabled = enabled
        app.mapView:setEnabled("item_inspector", enabled)
    end

    dialog:newrow()
    dialog:separator()
    dialog:check({
        text = "Enable Item Inspector Tooltip",
        selected = false,
        onclick = function(d)
            -- Use d.data to access widget values
            local checked = d.data and d.data.inspector_check
            updateInspector(checked)
        end,
        id = "inspector_check"
    })
end

dialog:show()

