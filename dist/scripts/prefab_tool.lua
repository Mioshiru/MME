-- Mios Map Editor: Prefab Capture Tool
-- Erfasst Bereiche wie eine PZ-Zone

local capture_start = nil
local is_capturing = false

local available_icons = {
    { name = "Hammer", id = "HAMMER" },
    { name = "Bett", id = "BED" },
    { name = "Rucksack", id = "BACKPACK" },
    { name = "Zauberstab", id = "WAND" },
    { name = "Hähnchen", id = "FOOD" }
}

-- Registriere Tool im Menü
editor.registerMenu("Tools/Prefab/Start Capture", function()
    is_capturing = true
    print("Prefab Capture gestartet: Klicke auf die Map zum Starten...")
end)

-- Event-Hooks für das Ziehen
function onMousePress(x, y, z, button)
    if is_capturing and button == "left" then
        capture_start = {x = x, y = y, z = z}
        print("Startpunkt gesetzt bei: " .. x .. ", " .. y)
        return true -- Event konsumieren
    end
end

function onMouseRelease(x, y, z, button)
    if is_capturing and capture_start and button == "left" then
        local x1 = math.min(capture_start.x, x)
        local y1 = math.min(capture_start.y, y)
        local w = math.abs(x - capture_start.x) + 1
        local h = math.abs(y - capture_start.y) + 1
        
        -- Prefab via API erfassen (Base64)
        local prefab_data = prefab.capture(x1, y1, w, h)
        
        -- Dialog für Name und Icon
        local dlg = Dialog({title = "Prefab Speichern", width = 250})
        dlg:input({id = "name", label = "Name:", text = "Gebäude #" .. os.date("%M%S")})
        
        local iconItems = {}
        for _, icon in ipairs(available_icons) do table.insert(iconItems, icon.name) end
        dlg:combobox({id = "icon", label = "Icon:", options = iconItems})
        
        dlg:button({text = "In Palette pinnen", onclick = function(d)
            local vals = d.data
            local iconId = available_icons[d.widget("icon"):getSelection() + 1].id
            
            -- Registriere in der permanenten Palette
            prefab.addToPalette(vals.name, prefab_data, iconId)
            
            -- Export-String für Foren in die Konsole werfen
            print("--- FORUM EXPORT START ---")
            print(prefab_data)
            print("--- FORUM EXPORT END ---")
            
            app.alert("Prefab '" .. vals.name .. "' wurde zur Palette hinzugefügt!")
            d:close()
        end})
        
        dlg:show()
        
        is_capturing = false
        capture_start = nil
        return true
    end
end

-- Overlay-Feedback (Optional: Zeichnet Rahmen beim Ziehen)
function onMapOverlay()
    if is_capturing and capture_start then
        local mx, my = editor.getMouseMapPos()
        local x = math.min(capture_start.x, mx)
        local y = math.min(capture_start.y, my)
        local w = math.abs(mx - capture_start.x) + 1
        local h = math.abs(my - capture_start.y) + 1
        
        -- Zeichne einen goldenen Rahmen
        overlay.drawRect(x, y, w, h, {r=218, g=165, b=32, a=150}, 2)
    end
end