-- Mio's Map Editor - Nature Scatter Script
-- Verteilt zufällige Natur-Details auf der markierten Fläche.

editor.registerScript("Nature Scatter", function()
    if not editor.hasSelection() then
        error("Bitte markiere zuerst eine Fläche auf der Karte!")
        return
    end

    -- IDs für Details (Beispiele für Standard-Assets)
    local stones = {3611, 3612, 3613, 3614}
    local grass = {621, 621, 621, 4580} -- Häufigkeit durch Duplikate steuerbar
    local flowers = {2743, 2767}

    local area = editor.getSelection()
    editor.beginBatchChange()

    local count = 0
    for _, pos in ipairs(area) do
        local tile = map.getTile(pos)
        -- Nur auf begehbarem Boden ohne Items verteilen
        if tile and tile:hasGround() and not tile:isBlocking() and tile:getItemCount() == 0 then
            local rand = math.random(1, 100)
            
            if rand < 15 then -- 15% Chance auf Gras/Details
                local detailID = 0
                if rand < 10 then
                    detailID = grass[math.random(#grass)]
                elseif rand < 13 then
                    detailID = stones[math.random(#stones)]
                else
                    detailID = flowers[math.random(#flowers)]
                end
                
                if detailID > 0 then
                    tile:addItem(Item.create(detailID))
                    count = count + 1
                end
            end
        end
    end

    editor.endBatchChange()
    g_gui.RefreshView()
    g_gui.setStatusText(string.format("Nature Scatter: %d Details verteilt.", count))
end)

editor.setScriptShortcut("Nature Scatter", "Shift+N")