#include "main.h"
#include "gui.h"
#include "settings.h"
#include "brush.h"

void GUI::SetHotkey(int index, Hotkey &hotkey) {
  ASSERT(index >= 0 && index <= 9);
  hotkeys[index] = hotkey;
  SetStatusText("Set hotkey " + i2ws(index) + ".");
}

const Hotkey &GUI::GetHotkey(int index) const {
  ASSERT(index >= 0 && index <= 9);
  return hotkeys[index];
}

void GUI::SaveHotkeys() const {
  std::ostringstream os;
  for (const auto &hotkey : hotkeys) {
    os << hotkey << '\n';
  }
  g_settings.setString(Config::NUMERICAL_HOTKEYS, os.str());
}

void GUI::LoadHotkeys() {
  std::istringstream is;
  is.str(g_settings.getString(Config::NUMERICAL_HOTKEYS));

  std::string line;
  int index = 0;
  while (getline(is, line)) {
    std::istringstream line_is;
    line_is.str(line);
    line_is >> hotkeys[index];

    ++index;
  }
}

Hotkey::Hotkey() : type(NONE) {
  ////
}

Hotkey::Hotkey(Position _pos) : type(POSITION), pos(_pos) {
  ////
}

Hotkey::Hotkey(Brush *brush) : type(BRUSH), brushname(brush->getName()) {
  ////
}

Hotkey::Hotkey(std::string _name) : type(BRUSH), brushname(_name) {
  ////
}

Hotkey::~Hotkey() {
  ////
}

std::ostream &operator<<(std::ostream &os, const Hotkey &hotkey) {
  switch (hotkey.type) {
  case Hotkey::POSITION: {
    os << "pos:{" << hotkey.pos << "}";
  } break;
  case Hotkey::BRUSH: {
    if (hotkey.brushname.find('{') != std::string::npos ||
        hotkey.brushname.find('}') != std::string::npos) {
      break;
    }
    os << "brush:{" << hotkey.brushname << "}";
  } break;
  default: {
    os << "none:{}";
  } break;
  }
  return os;
}

std::istream &operator>>(std::istream &is, Hotkey &hotkey) {
  std::string type;
  getline(is, type, ':');
  if (type == "none") {
    is.ignore(2); // ignore "{}"
  } else if (type == "pos") {
    is.ignore(1); // ignore "{"
    Position pos;
    is >> pos;
    hotkey = Hotkey(pos);
    is.ignore(1); // ignore "}"
  } else if (type == "brush") {
    is.ignore(1); // ignore "{"
    std::string brushname;
    getline(is, brushname, '}');
    hotkey = Hotkey(brushname);
  } else {
    // Do nothing...
  }

  return is;
}