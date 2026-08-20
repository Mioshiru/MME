-- Mio's Map Editor - Shop Generator Script
-- Erzeugt ein Gebäude mit Tresen und Einrichtung basierend auf der aktuellen Auswahl.

local function generateShop(shopType)
    local pos = editor.getCursorPosition()
    local groundBrush = g_gui.getCurrentBrush()
    
    if not groundBrush or not groundBrush:isGround() then
        error("Bitte wähle zuerst einen Boden-Typ (Ground Brush) in der Palette aus!")
        return
    end

    local width = 7
    local height = 6
    local wallBrush = g_brushes.getBrush("Stone Wall") or g_brushes.getBrush("Wall")
    
    -- Shop Konfigurationen
    local configs = {
        ["Weapons"]  = { counter = 1611, deco = {3358, 3350, 3362}, name = "Waffenschmied" },
        ["Armor"]    = { counter = 1611, deco = {3355, 3351, 3354}, name = "Rüstungsladen" },
        ["Magic"]    = { counter = 1612, deco = {2033, 2117, 2185}, name = "Alchemist" },
        ["Food"]     = { counter = 1613, deco = {1767, 1770, 1780}, name = "Nahrungshändler" },
        ["Jewelry"]  = { counter = 1612, deco = {3007, 3010, 3012}, name = "Juwelier" },
        ["Library"]  = { counter = 1611, deco = {1952, 1953, 1954}, name = "Bibliothek" },
        ["Tools"]    = { counter = 1611, deco = {2550, 2553, 2554, 2557}, name = "Ausrüstungsladen" },
        ["Furniture"] = { counter = 1611, deco = {1614, 1615, 1616, 1619}, name = "Möbelhaus" }
    }
    local config = configs[shopType] or configs["Weapons"]

    editor.beginBatchChange()
    for dx = 0, width - 1 do
        for dy = 0, height - 1 do
            editor.draw(Position(pos.x + dx, pos.y + dy, pos.z))
        end
    end
    if wallBrush then
        g_gui.selectBrush(wallBrush)
        for dx = 0, width - 1 do
            editor.draw(Position(pos.x + dx, pos.y, pos.z)) -- Nord
            editor.draw(Position(pos.x + dx, pos.y + height - 1, pos.z)) -- Süd
        end
        for dy = 0, height - 1 do
            editor.draw(Position(pos.x, pos.y + dy, pos.z)) -- West
            editor.draw(Position(pos.x + width - 1, pos.y + dy, pos.z)) -- Ost
        end
    end
    local doorPos = Position(pos.x + math.floor(width/2), pos.y + height - 1, pos.z)
    local doorBrush = g_gui.normal_door_brush
    if doorBrush then g_gui.selectBrush(doorBrush) editor.draw(doorPos) end

    for dx = 1, width - 2 do
        local item = Item.create(config.counter)
        local counterPos = Position(pos.x + dx, pos.y + 2, pos.z)
        local tile = map.getOrCreateTile(counterPos)
        tile:addItem(item)
    end

    for dx = 1, width - 2 do
        local tile = map.getOrCreateTile(Position(pos.x + dx, pos.y + 1, pos.z))
        if math.random(1, 10) > 4 then
            tile:addItem(Item.create(config.deco[math.random(#config.deco)]))
        end
    end
    editor.endBatchChange()
    g_gui.selectBrush(groundBrush)
    g_gui.setStatusText(string.format("%s erfolgreich generiert!", config.name))
end

-- Registrierung der verschiedenen Typen als Untermenü-Struktur
editor.registerScript("Shop Gen/Waffenladen", function()
    generateShop("Weapons")
end)

editor.registerScript("Shop Gen/Rüstungsladen", function()
    generateShop("Armor")
end)

editor.registerScript("Shop Gen/Alchemist", function()
    generateShop("Magic")
end)

editor.registerScript("Shop Gen/Nahrungsmittel", function()
    generateShop("Food")
end)

editor.registerScript("Shop Gen/Juwelier", function()
    generateShop("Jewelry")
end)

editor.registerScript("Shop Gen/Bibliothek", function()
    generateShop("Library")
end)

editor.registerScript("Shop Gen/Werkzeuge", function()
    generateShop("Tools")
end)

editor.registerScript("Shop Gen/Möbel", function()
    generateShop("Furniture")
end)