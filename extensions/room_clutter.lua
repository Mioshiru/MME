-- Mio's Map Editor - Auto Room Clutter
-- Fügt leeren Räumen Details wie Staub und Spinnweben hinzu.

editor.registerScript("Auto-Room Clutter", function()
    if not editor.hasSelection() then
        error("Bitte markiere den Raum!")
        return
    end

    local dustID = 2246
    local webIDs = {1445, 1447}
    
    local area = editor.getSelection()
    editor.beginBatchChange()
    
    local count = 0
    for _, pos in ipairs(area) do
        local tile = map.getTile(pos)
        -- Wir suchen nach Kacheln, die Boden aber keine Wände haben (Inneres)
        if tile and tile:hasGround() and not tile:hasWall() then
            local rand = math.random(1, 100)
            
            -- Spinnweben in Ecken (Simuliert durch Nachbarschaftscheck)
            if rand < 5 then
                tile:addItem(Item.create(webIDs[math.random(#webIDs)]))
                count = count + 1
            elseif rand < 12 then
                -- Staub / Dreck auf dem Boden
                tile:addItem(Item.create(dustID))
                count = count + 1
            end
        end
    end
    
    editor.endBatchChange()
    g_gui.RefreshView()
    g_gui.setStatusText("Raum-Details hinzugefügt.")
end)