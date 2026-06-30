using System;
using System.Collections.Generic;
using System.Numerics;
using ImGuiNET;
using MapEditor.Core.Actions;
using MapEditor.Core.State;
using MapEditor.Assets;
using MapEditor.Assets.Atlas;

namespace MapEditor.UI.Windows;

public sealed class PaletteView
{
    private readonly EditorStore _store;
    private readonly PaletteViewModel _viewModel;
    private readonly AssetManager _assetManager;

    public PaletteView(EditorStore store, PaletteViewModel viewModel, AssetManager assetManager)
    {
        _store = store;
        _viewModel = viewModel;
        _assetManager = assetManager;
    }

    public void Render(EditorState state)
    {
        if (!_viewModel.IsDataLoaded)
        {
            if (_assetManager.IsReady)
            {
                _viewModel.LoadData();
            }
        }

        // It's a dockable window now.
        var flags = ImGuiWindowFlags.NoCollapse; 

        if (!ImGui.Begin("Terrain Palette", flags))
        {
            ImGui.End();
            return;
        }

        if (!_viewModel.IsDataLoaded || _viewModel.Tilesets.Count == 0)
        {
            ImGui.TextColored(new Vector4(1f, 0.4f, 0.4f, 1f), "Loading assets...");
            ImGui.End();
            return;
        }

        // 1. First-Stage Filter: Tileset selection combo
        ImGui.Text("Tileset:");
        ImGui.SetNextItemWidth(-1);
        string currentTilesetName = _viewModel.Tilesets[_viewModel.SelectedTilesetIndex].Name;
        if (ImGui.BeginCombo("##TilesetCombo", currentTilesetName))
        {
            for (int i = 0; i < _viewModel.Tilesets.Count; i++)
            {
                bool isSelected = (i == _viewModel.SelectedTilesetIndex);
                if (ImGui.Selectable(_viewModel.Tilesets[i].Name, isSelected))
                {
                    _viewModel.SelectedTilesetIndex = i;
                }
                if (isSelected)
                {
                    ImGui.SetItemDefaultFocus();
                }
            }
            ImGui.EndCombo();
        }

        ImGui.Spacing();

        // Search Filter
        ImGui.SetNextItemWidth(-1);
        string query = _viewModel.SearchQuery;
        if (ImGui.InputTextWithHint("##ItemSearch", "Search items...", ref query, 64))
        {
            _viewModel.SearchQuery = query;
        }

        ImGui.Separator();
        ImGui.Spacing();

        // 2. Second-Stage: Items inside current tileset
        var activeTileset = _viewModel.Tilesets[_viewModel.SelectedTilesetIndex];
        List<ushort> filteredIds = new();

        foreach (ushort id in activeTileset.ItemIds)
        {
            string name = _viewModel.ItemNames.TryGetValue(id, out var n) ? n : "Unnamed";
            if (string.IsNullOrWhiteSpace(_viewModel.SearchQuery) ||
                id.ToString().Contains(_viewModel.SearchQuery, StringComparison.OrdinalIgnoreCase) ||
                name.Contains(_viewModel.SearchQuery, StringComparison.OrdinalIgnoreCase))
            {
                filteredIds.Add(id);
            }
        }

        if (filteredIds.Count == 0)
        {
            ImGui.TextDisabled("No items match query.");
        }
        else
        {
            ImGui.BeginChild("##ItemGridChild", new Vector2(0, 0), ImGuiChildFlags.None);

            float availW   = ImGui.GetContentRegionAvail().X;
            int iconSize   = 32;
            int padding    = 4;
            int cols       = Math.Max(1, (int)(availW / (iconSize + padding)));
            int col        = 0;

            foreach (ushort id in filteredIds)
            {
                bool selected = (state.SelectedItemId == id);

                if (selected)
                {
                    ImGui.PushStyleColor(ImGuiCol.Button, new Vector4(0.20f, 0.45f, 0.80f, 1f));
                }

                ImGui.PushID(id);
                bool clicked = false;
                var info = _assetManager.DatDb?.GetItem(id);
                uint spriteId = info?.GetSpriteId(0, 0, 0, 0, 0, 0, 0) ?? 0;
                
                if (_assetManager.UiAtlas != null && _assetManager.UiAtlas.IsBuilt && spriteId != 0 && _assetManager.UiAtlas.GetUV(spriteId) is SpriteUV uv)
                {
                    clicked = ImGui.ImageButton($"##btn_{id}", (IntPtr)_assetManager.UiAtlas.Handle, new Vector2(iconSize, iconSize), new Vector2(uv.U0, uv.V0), new Vector2(uv.U1, uv.V1));
                }
                else
                {
                    clicked = ImGui.Button($"{id}", new Vector2(iconSize, iconSize));
                }
                ImGui.PopID();

                if (selected)
                {
                    ImGui.PopStyleColor();
                }

                // Show item name and attributes in Tooltip on hover
                if (ImGui.IsItemHovered())
                {
                    string name = _viewModel.ItemNames.TryGetValue(id, out var n) ? n : "Unnamed";
                    if (info != null)
                    {
                        ImGui.SetTooltip(
                            $"ID: {id}\n" +
                            $"Name: {name}\n" +
                            $"Flags: {info.Flags}\n" +
                            $"Size: {info.Width}×{info.Height}");
                    }
                    else
                    {
                        ImGui.SetTooltip($"ID: {id}\nName: {name}");
                    }
                }

                if (clicked)
                {
                    _store.Dispatch(new SelectItemAction(id));
                }

                col++;
                if (col < cols)
                {
                    ImGui.SameLine();
                }
                else
                {
                    col = 0;
                }
            }

            ImGui.EndChild();
        }

        ImGui.End();
    }
}
