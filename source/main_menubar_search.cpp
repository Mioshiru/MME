#include "main.h"
#include "editor.h"
#include "find_item_window.h"
#include "gui.h"
#include "main_menubar.h"
#include "map_tab.h"
#include "map_window.h"
#include "result_window.h"
#include "settings.h"
#include "tile.h"
#include "item.h"
#include <utility>


namespace OnSearchForItem {
struct Finder {
  Finder(uint16_t itemId, uint32_t maxCount)
      : itemId(itemId), maxCount(maxCount) {}

  uint16_t itemId;
  uint32_t maxCount;
  std::vector<std::pair<Tile *, Item *>> result;

  bool limitReached() const { return result.size() >= (size_t)maxCount; }

  void operator()(Map &map, Tile *tile, Item *item, long long done) {
    if (result.size() >= (size_t)maxCount) {
      return;
    }

    if (done % 0x8000 == 0) {
      g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
    }

    if (item->getID() == itemId) {
      result.push_back(std::make_pair(tile, item));
    }
  }
};
} // namespace OnSearchForItem

void MainMenuBar::OnSearchForItem(wxCommandEvent &WXUNUSED(event)) {
  if (!g_gui.IsEditorOpen()) {
    return;
  }

  FindItemDialog dialog(frame, "Search for Item");
  dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(
      Config::FIND_ITEM_MODE));
  if (dialog.ShowModal() == wxID_OK) {
    OnSearchForItem::Finder finder(
        dialog.getResultID(),
        (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));
    g_gui.CreateLoadBar("Searching map...");

    foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, false);
    std::vector<std::pair<Tile *, Item *>> &result = finder.result;

    g_gui.DestroyLoadBar();

    if (finder.limitReached()) {
      wxString msg;
      msg << "The configured limit has been reached. Only " << finder.maxCount
          << " results will be displayed.";
      g_gui.PopupDialog("Notice", msg, wxOK);
    }

    SearchResultWindow *window = g_gui.ShowSearchWindow();
    window->Clear();
    for (std::vector<std::pair<Tile *, Item *>>::const_iterator iter =
             result.begin();
         iter != result.end(); ++iter) {
      Tile *tile = iter->first;
      Item *item = iter->second;
      window->AddPosition(wxstr(item->getName()), tile->getPosition());
    }

    g_settings.setInteger(Config::FIND_ITEM_MODE, (int)dialog.getSearchMode());
  }
  dialog.Destroy();
}

void MainMenuBar::OnReplaceItems(wxCommandEvent &WXUNUSED(event)) {
  if (!g_gui.IsVersionLoaded()) {
    return;
  }

  if (MapTab *tab = g_gui.GetCurrentMapTab()) {
    if (MapWindow *window = tab->GetView()) {
      window->ShowReplaceItemsDialog(false);
    }
  }
}

namespace OnSearchForStuff {
struct Searcher {
  Searcher()
      : search_unique(false), search_action(false), search_container(false),
        search_writeable(false) {}

  bool search_unique;
  bool search_action;
  bool search_container;
  bool search_writeable;
  std::vector<std::pair<Tile *, Item *>> found;

  void operator()(Map &map, Tile *tile, Item *item, long long done) {
    if (done % 0x8000 == 0) {
      g_gui.SetLoadDone((unsigned int)(100 * done / map.getTileCount()));
    }
    Container *container;
    if ((search_unique && item->getUniqueID() > 0) ||
        (search_action && item->getActionID() > 0) ||
        (search_container && ((container = dynamic_cast<Container *>(item)) &&
                              container->getItemCount())) ||
        (search_writeable && item->getText().length() > 0)) {
      found.push_back(std::make_pair(tile, item));
    }
  }

  wxString desc(Item *item) {
    wxString label;
    if (search_action) {
      if (item->getActionID() > 0) {
        label << "AID:" << item->getActionID() << " ";
      }
      if (item->getUniqueID() > 0) {
        label << "UID:" << item->getUniqueID() << " ";
      }
    } else {
      if (item->getUniqueID() > 0) {
        label << "UID:" << item->getUniqueID() << " ";
      }
      if (item->getActionID() > 0) {
        label << "AID:" << item->getActionID() << " ";
      }
    }

    label << wxstr(item->getName());

    if (dynamic_cast<Container *>(item)) {
      label << " (Container) ";
    }

    if (item->getText().length() > 0) {
      label << " (Text: " << wxstr(item->getText()) << ") ";
    }

    return label;
  }

  void sort() {
    if (search_unique && !search_action) {
      std::sort(found.begin(), found.end(),
                [](const std::pair<Tile *, Item *> &pair1,
                   const std::pair<Tile *, Item *> &pair2) {
                  const Item *item1 = pair1.second;
                  const Item *item2 = pair2.second;

                  uint16_t u1 = item1->getUniqueID();
                  uint16_t u2 = item2->getUniqueID();
                  if (u1 != u2) {
                    return u1 < u2;
                  }
                  return item1->getActionID() < item2->getActionID();
                });
    } else if (search_action && !search_unique) {
      std::sort(found.begin(), found.end(),
                [](const std::pair<Tile *, Item *> &pair1,
                   const std::pair<Tile *, Item *> &pair2) {
                  const Item *item1 = pair1.second;
                  const Item *item2 = pair2.second;

                  uint16_t a1 = item1->getActionID();
                  uint16_t a2 = item2->getActionID();
                  if (a1 != a2) {
                    return a1 < a2;
                  }
                  return item1->getUniqueID() < item2->getUniqueID();
                });
    } else if (search_unique || search_action) {
      std::sort(found.begin(), found.end(), Searcher::compare);
    }
  }

  static bool compare(const std::pair<Tile *, Item *> &pair1,
                      const std::pair<Tile *, Item *> &pair2) {
    const Item *item1 = pair1.second;
    const Item *item2 = pair2.second;

    if (item1->getActionID() != 0 || item2->getActionID() != 0) {
      return item1->getActionID() < item2->getActionID();
    } else if (item1->getUniqueID() != 0 || item2->getUniqueID() != 0) {
      return item1->getUniqueID() < item2->getUniqueID();
    }

    return false;
  }
};
} // namespace OnSearchForStuff

void MainMenuBar::OnSearchForStuffOnMap(wxCommandEvent &WXUNUSED(event)) {
  SearchItems(true, true, true, true);
}

void MainMenuBar::OnSearchForUniqueOnMap(wxCommandEvent &WXUNUSED(event)) {
  SearchItems(true, false, false, false);
}

void MainMenuBar::OnSearchForActionOnMap(wxCommandEvent &WXUNUSED(event)) {
  SearchItems(false, true, false, false);
}

void MainMenuBar::OnSearchForContainerOnMap(wxCommandEvent &WXUNUSED(event)) {
  SearchItems(false, false, true, false);
}

void MainMenuBar::OnSearchForWriteableOnMap(wxCommandEvent &WXUNUSED(event)) {
  SearchItems(false, false, false, true);
}

void MainMenuBar::OnSearchForStuffOnSelection(wxCommandEvent &WXUNUSED(event)) {
  SearchItems(true, true, true, true, true);
}

void MainMenuBar::OnSearchForUniqueOnSelection(
    wxCommandEvent &WXUNUSED(event)) {
  SearchItems(true, false, false, false, true);
}

void MainMenuBar::OnSearchForActionOnSelection(
    wxCommandEvent &WXUNUSED(event)) {
  SearchItems(false, true, false, false, true);
}

void MainMenuBar::OnSearchForContainerOnSelection(
    wxCommandEvent &WXUNUSED(event)) {
  SearchItems(false, false, true, false, true);
}

void MainMenuBar::OnSearchForWriteableOnSelection(
    wxCommandEvent &WXUNUSED(event)) {
  SearchItems(false, false, false, true, true);
}

void MainMenuBar::OnSearchForItemOnSelection(wxCommandEvent &WXUNUSED(event)) {
  if (!g_gui.IsEditorOpen()) {
    return;
  }
  FindItemDialog dialog(frame, "Search on Selection");
  dialog.setSearchMode((FindItemDialog::SearchMode)g_settings.getInteger(
      Config::FIND_ITEM_MODE));
  if (dialog.ShowModal() == wxID_OK) {
    OnSearchForItem::Finder finder(
        dialog.getResultID(),
        (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));
    g_gui.CreateLoadBar("Searching on selected area...");
    foreach_ItemOnMap(g_gui.GetCurrentMap(), finder, true);
    std::vector<std::pair<Tile *, Item *>> &result = finder.result;
    g_gui.DestroyLoadBar();
    if (finder.limitReached()) {
      wxString msg;
      msg << "The configured limit has been reached. Only " << finder.maxCount
          << " results will be displayed.";
      g_gui.PopupDialog("Notice", msg, wxOK);
    }
    SearchResultWindow *window = g_gui.ShowSearchWindow();
    window->Clear();
    for (std::vector<std::pair<Tile *, Item *>>::const_iterator iter =
             result.begin();
         iter != result.end(); ++iter) {
      window->AddPosition(wxstr(iter->second->getName()),
                          iter->first->getPosition());
    }
    g_settings.setInteger(Config::FIND_ITEM_MODE, (int)dialog.getSearchMode());
  }
  dialog.Destroy();
}

void MainMenuBar::OnReplaceItemsOnSelection(wxCommandEvent &WXUNUSED(event)) {
  if (!g_gui.IsVersionLoaded())
    return;
  if (MapTab *tab = g_gui.GetCurrentMapTab()) {
    if (MapWindow *window = tab->GetView()) {
      window->ShowReplaceItemsDialog(true);
    }
  }
}

void MainMenuBar::OnRemoveItemOnSelection(wxCommandEvent &WXUNUSED(event)) {
  if (!g_gui.IsEditorOpen())
    return;
  FindItemDialog dialog(frame, "Remove Item on Selection");
  if (dialog.ShowModal() == wxID_OK) {
    g_gui.GetCurrentEditor()->actionQueue->clear();
    g_gui.CreateLoadBar("Searching item on selection to remove...");

    auto removeCondition = [itemId = dialog.getResultID()](Map &map, Item *item,
                                                           int64_t removed,
                                                           int64_t done) {
      if (done % 0x8000 == 0) {
        g_gui.SetLoadDone((uint32_t)(100 * done / map.getTileCount()));
      }
      return item->getID() == itemId && !item->isComplex();
    };
    int64_t count =
        RemoveItemOnMap(g_gui.GetCurrentMap(), removeCondition, true);
    g_gui.DestroyLoadBar();
    wxString msg;
    msg << count << " items removed.";
    g_gui.PopupDialog("Remove Item", msg, wxOK);
    g_gui.GetCurrentMap().doChange();
    g_gui.RefreshView();
  }
  dialog.Destroy();
}

void MainMenuBar::SearchItems(bool unique, bool action, bool container,
                              bool writable, bool onSelection /* = false*/) {
  if (!unique && !action && !container && !writable)
    return;
  if (!g_gui.IsEditorOpen())
    return;

  if (onSelection)
    g_gui.CreateLoadBar("Searching on selected area...");
  else
    g_gui.CreateLoadBar("Searching on map...");

  OnSearchForStuff::Searcher searcher;
  searcher.search_unique = unique;
  searcher.search_action = action;
  searcher.search_container = container;
  searcher.search_writeable = writable;

  foreach_ItemOnMap(g_gui.GetCurrentMap(), searcher, onSelection);
  searcher.sort();
  std::vector<std::pair<Tile *, Item *>> &found = searcher.found;

  g_gui.DestroyLoadBar();

  SearchResultWindow *result = g_gui.ShowSearchWindow();
  result->Clear();
  for (std::vector<std::pair<Tile *, Item *>>::iterator iter = found.begin();
       iter != found.end(); ++iter) {
    result->AddPosition(searcher.desc(iter->second),
                        iter->first->getPosition());
  }
}