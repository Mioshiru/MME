-- @Title: Prefab Manager & History
-- @Author: Gemini Code Assist
-- @Description: Clipboard-Manager für Prefabs mit Ghost-Vorschau.

local history = {}
local max_history = 10
local active_prefab = nil

-- Hilfsfunktion: Kürzt lange Base64 Strings für die UI
local function shortLabel(str)
    return str:sub(1, 15) .. "..." .. str:sub(-5)
end

local function showPrefabUI()
    local dlg = Dialog({title = "Prefab History", width = 300})
    
    dlg:label({text = "Zuletzt kopierte Gebäude:"})
    
    local listItems = {}
    for i, p in ipairs(history) do
        table.insert(listItems, {text = i .. ". Prefab (" .. shortLabel(p) .. ")"})
    end
    
    dlg:list({
        id = "history_list",
        items = listItems,
        onchange = function(d, index)
            active_prefab = history[index]
            prefab.setGhost(active_prefab)
            print("Ghost-Vorschau aktiviert für Slot " .. index)
        end,
        ondoubleclick = function(d, index)
            local x, y = editor.getMouseMapPos()
            prefab.place(x, y, history[index])
            d:close()
        end
    })

    dlg:wrap()
        dlg:button({text = "Aus Clipboard laden", onclick = function()
            -- Hier nutzen wir die neue C++ Funktion
            if prefab.importFromClipboard then
                -- Wir holen uns das Prefab nur in den Ghost-Mode statt direkt zu platzieren
                -- (Erfordert app.setClipboard/getClipboard Support)
                app.alert("Kopiere einen Prefab-String und wähle 'Ghost Mode'")
            end
        end})
        
        dlg:button({text = "In Palette pinnen", onclick = function()
            if active_prefab then
                local name = app.prompt("Name für das Prefab:", "Neues Gebäude")
                if name then prefab.addToPalette(name, active_prefab) end
            end
        end})

        dlg:button({text = "Ghost löschen", onclick = function()
            prefab.clearGhost()
            active_prefab = nil
        end})
    dlg:endwrap()

    dlg:show({wait = false})
end

-- Hook um neue Captures automatisch in die History zu packen
local old_capture = prefab.capture
prefab.capture = function(x, y, w, h)
    local data = old_capture(x, y, w, h)
    table.insert(history, 1, data)
    if #history > max_history then table.remove(history) end
    return data
end

-- Hotkey 'V' für History-Menü (wenn im Auswahlmodus)
function onKeyDown(key)
    if key == "v" or key == "V" then
        if not app.keyboard.isCtrlDown() then -- Normales Paste nicht überschreiben
            showPrefabUI()
            return true
        end
    elseif key == "Escape" then
        prefab.clearGhost()
    end
end

editor.registerMenu("Tools/Prefab/History", showPrefabUI)