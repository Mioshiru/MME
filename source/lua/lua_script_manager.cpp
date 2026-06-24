#include "main.h"
#include "lua_script_manager.h"
#include "gui.h"
#include <sol/sol.hpp>


LuaScriptManager &LuaScriptManager::getInstance() {
  static LuaScriptManager instance;
  return instance;
}

void LuaScriptManager::logOutput(const std::string &message, bool isError) {
  // Weiterleitung an die neue ImGui Debug Konsole
  g_gui.AddDebugLog(message, isError);
}

bool LuaScriptManager::initialize() {
  engine.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string,
                        sol::lib::math, sol::lib::table, sol::lib::os,
                        sol::lib::coroutine);
  sol::state &lua = engine;

  // print() umleiten
  lua["print"] = [this](sol::variadic_args va) {
    std::string s;
    for (auto v : va) {
      s += v.as<std::string>() + " ";
    }
    logOutput(s, false);
  };

  registerAPIs();
  initialized = true;
  return true;
}

void LuaScriptManager::registerAPIs() {
  sol::state &lua = engine;

  // Fehlerbehandlung für geschützte Aufrufe (pcall)
  lua.set_exception_handler(
      [this](lua_State *L,
             sol::optional<const std::exception &> maybe_exception,
             sol::string_view description) {
        std::string errMsg = "Lua Runtime Error: ";
        errMsg += description.data();
        logOutput(errMsg, true);
        return sol::stack::push(L, description);
      });

  // ... weitere API Registrierungen ...
}