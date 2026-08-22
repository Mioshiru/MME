-- Mio's Map Editor - Lighting Wizard
-- Automatische Zuweisung von Lichtquellen für RPG-Items

local lightConfigs = {
    [2106] = {range = 3, color = 215}, -- Wandfackel
    [2117] = {range = 2, color = 215}, -- Kerzenleuchter
    [2033] = {range = 1, color = 215}, -- Kleine Kerze
    [2041] = {range = 4, color = 200}, -- Große Lampe
}

editor.registerScript("Lighting Wizard", function()
    local area = editor.hasSelection() and editor.getSelection() or nil
    if not area then
        error("Bitte markiere zuerst einen Bereich!")
        return
    end

    editor.beginBatchChange()
    local count = 0

    for _, pos in ipairs(area) do
        local tile = map.getTile(pos)
        if tile then
            for i = 0, tile:getItemCount() - 1 do
                local item = tile:getItemAt(i)
                local config = lightConfigs[item:getId()]
                
                if config then
                    -- Setze Lichtwerte wenn noch kein Licht vorhanden ist
                    item:setLight(config.range, config.color)
                    count = count + 1
                end
            end
        end
    end

    editor.endBatchChange()
    g_gui.RefreshView()
    g_gui.setStatusText(string.format("Lighting Wizard: %d Lichtquellen konfiguriert.", count))
end)

editor.setScriptShortcut("Lighting Wizard", "Shift+L")