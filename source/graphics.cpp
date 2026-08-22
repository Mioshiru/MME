//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
// Optimized for High FPS & Cache Locality
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "async_loader.h"
#include "filehandle.h"
#include "graphics.h"
#include "gui.h"
#include "otml.h"
#include "settings.h"
#include "sprites.h"
#include <wx/glcanvas.h>

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#include <nanovg.h>

#include "pngfiles.h"
#include <format>
#include <wx/dir.h>
#include <wx/mstream.h>
#include <wx/stopwatch.h>

#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif

static uint32_t TemplateOutfitLookupTable[] = {
    0xFFFFFF, 0xFFD4BF, 0xFFE9BF, 0xFFFFBF, 0xE9FFBF, 0xD4FFBF, 0xBFFFBF,
    0xBFFFD4, 0xBFFFE9, 0xBFFFFF, 0xBFE9FF, 0xBFD4FF, 0xBFBFFF, 0xD4BFFF,
    0xE9BFFF, 0xFFBFFF, 0xFFBFE9, 0xFFBFD4, 0xFFBFBF, 0xDADADA, 0xBF9F8F,
    0xBFAF8F, 0xBFBF8F, 0xAFBF8F, 0x9FBF8F, 0x8FBF8F, 0x8FBF9F, 0x8FBFAF,
    0x8FBFBF, 0x8FAFBF, 0x8F9FBF, 0x8F8FBF, 0x9F8FBF, 0xAF8FBF, 0xBF8FBF,
    0xBF8FAF, 0xBF8F9F, 0xBF8F8F, 0xB6B6B6, 0xBF7F5F, 0xBFAF8F, 0xBFBF5F,
    0x9FBF5F, 0x7FBF5F, 0x5FBF5F, 0x5FBF7F, 0x5FBF9F, 0x5FBFBF, 0x5F9FBF,
    0x5F7FBF, 0x5F5FBF, 0x7F5FBF, 0x9F5FBF, 0xBF5FBF, 0xBF5F9F, 0xBF5F7F,
    0xBF5F5F, 0x919191, 0xBF6A3F, 0xBF943F, 0xBFBF3F, 0x94BF3F, 0x6ABF3F,
    0x3FBF3F, 0x3FBF6A, 0x3FBF94, 0x3FBFBF, 0x3F94BF, 0x3F6ABF, 0x3F3FBF,
    0x6A3FBF, 0x943FBF, 0xBF3FBF, 0xBF3F94, 0xBF3F6A, 0xBF3F3F, 0x6D6D6D,
    0xFF5500, 0xFFAA00, 0xFFFF00, 0xAAFF00, 0x54FF00, 0x00FF00, 0x00FF54,
    0x00FFAA, 0x00FFFF, 0x00A9FF, 0x0055FF, 0x0000FF, 0x5500FF, 0xA900FF,
    0xFE00FF, 0xFF00AA, 0xFF0055, 0xFF0000, 0x484848, 0xBF3F00, 0xBF7F00,
    0xBFBF00, 0x7FBF00, 0x3FBF00, 0x00BF00, 0x00BF3F, 0x00BF7F, 0x00BFBF,
    0x007FBF, 0x003FBF, 0x0000BF, 0x3F00BF, 0x7F00BF, 0xBF00BF, 0xBF007F,
    0xBF003F, 0xBF0000, 0x242424, 0x7F2A00, 0x7F5500, 0x7F7F00, 0x557F00,
    0x2A7F00, 0x007F00, 0x007F2A, 0x007F55, 0x007F7F, 0x00547F, 0x002A7F,
    0x00007F, 0x2A007F, 0x54007F, 0x7F007F, 0x7F0055, 0x7F002A, 0x7F0000};

GraphicManager::GraphicManager()
    : client_version(nullptr), unloaded(true), dat_format(DAT_FORMAT_UNKNOWN),
      otfi_found(false), is_extended(false), has_transparency(false),
      has_frame_durations(false), has_frame_groups(false), loaded_textures(0),
      lastclean(0) {
  animation_timer = newd wxStopWatch();
  animation_timer->Start();
}

GraphicManager::~GraphicManager() {
  for (auto &pair : sprite_space)
    delete pair.second;
  for (auto &pair : image_space)
    delete pair.second;
  delete animation_timer;
}

bool GraphicManager::hasTransparency() const { return has_transparency; }
bool GraphicManager::isUnloaded() const { return unloaded; }

GLuint GraphicManager::getFreeTextureID() {
  static GLuint id_counter = 0x10000000;
  return id_counter++;
}

NSVGimage *GraphicManager::loadSVG(const std::string &filename) {
  if (filename.find("..") != std::string::npos)
    return nullptr;
  return nsvgParseFromFile(filename.c_str(), "px", 96.0f);
}

void GraphicManager::renderSVG(NVGcontext *vg, NSVGimage *img, float x, float y,
                               float w, float h) {
  if (!vg || !img)
    return;
  nvgSave(vg);
  float scale = std::min(w / img->width, h / img->height);
  nvgTranslate(vg, x, y);
  nvgScale(vg, scale, scale);

  for (NSVGshape *shape = img->shapes; shape != NULL; shape = shape->next) {
    if (!(shape->flags & NSVG_FLAGS_VISIBLE))
      continue;
    for (NSVGpath *path = shape->paths; path != NULL; path = path->next) {
      nvgBeginPath(vg);
      nvgMoveTo(vg, path->pts[0], path->pts[1]);
      for (int i = 0; i < path->npts - 1; i += 3) {
        float *p = &path->pts[i * 2];
        nvgBezierTo(vg, p[2], p[3], p[4], p[5], p[6], p[7]);
      }
      if (path->closed)
        nvgClosePath(vg);

      if (shape->fill.type == NSVG_PAINT_COLOR) {
        uint32_t c = shape->fill.color;
        nvgFillColor(vg, nvgRGBA(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff,
                                 (c >> 24) & 0xff));
        nvgFill(vg);
      }
      if (shape->stroke.type == NSVG_PAINT_COLOR) {
        uint32_t c = shape->stroke.color;
        nvgStrokeColor(vg, nvgRGBA(c & 0xff, (c >> 8) & 0xff, (c >> 16) & 0xff,
                                   (c >> 24) & 0xff));
        nvgStrokeWidth(vg, shape->strokeWidth);
        nvgStroke(vg);
      }
    }
  }
  nvgRestore(vg);
}

void GraphicManager::clear() {
  SpriteMap new_sprite_space;
  for (auto &iter : sprite_space) {
    if (iter.first >= 0)
      delete iter.second;
    else
      new_sprite_space.insert(iter);
  }
  for (auto &iter : image_space)
    delete iter.second;

  sprite_space.swap(new_sprite_space);
  image_space.clear();
  cleanup_list.clear();

  item_count = 0;
  creature_count = 0;
  loaded_textures = 0;
  lastclean = time(nullptr);
  spritefile = "";
  unloaded = true;
}

void GraphicManager::cleanSoftwareSprites() {
  for (auto &iter : sprite_space) {
    if (iter.first >= 0)
      iter.second->unloadDC();
  }
}

Sprite *GraphicManager::getSprite(int id) {
  auto it = sprite_space.find(id);
  return (it != sprite_space.end()) ? it->second : nullptr;
}

GameSprite *GraphicManager::getCreatureSprite(int id) {
  if (id < 0)
    return nullptr;
  auto it = sprite_space.find(id + item_count);
  return (it != sprite_space.end()) ? static_cast<GameSprite *>(it->second)
                                    : nullptr;
}

uint16_t GraphicManager::getItemSpriteMaxID() const { return item_count; }
uint16_t GraphicManager::getCreatureSpriteMaxID() const {
  return creature_count;
}

#define loadPNGFile(name) _wxGetBitmapFromMemory(name, sizeof(name))
inline wxBitmap *_wxGetBitmapFromMemory(const unsigned char *data, int length) {
  wxMemoryInputStream is(data, length);
  wxImage img(is, "image/png");
  return img.IsOk() ? newd wxBitmap(img, -1) : nullptr;
}

inline wxBitmap *
LoadBitmapFromFile(const std::initializer_list<const char *> &paths) {
  for (const char *path : paths) {
    const wxString file(path);
    if (!wxFileExists(file))
      continue;
    wxImage img;
    if (img.LoadFile(file, wxBITMAP_TYPE_PNG))
      return newd wxBitmap(img, -1);
  }
  return nullptr;
}

bool GraphicManager::loadEditorSprites() {
  sprite_space[EDITOR_SPRITE_SELECTION_MARKER] =
      newd EditorSprite(newd wxBitmap(selection_marker_xpm16x16),
                        newd wxBitmap(selection_marker_xpm32x32));
  sprite_space[EDITOR_SPRITE_BRUSH_CD_1x1] = newd EditorSprite(
      loadPNGFile(circular_1_small_png), loadPNGFile(circular_1_png));
  sprite_space[EDITOR_SPRITE_BRUSH_CD_3x3] = newd EditorSprite(
      loadPNGFile(circular_2_small_png), loadPNGFile(circular_2_png));
  sprite_space[EDITOR_SPRITE_BRUSH_CD_5x5] = newd EditorSprite(
      loadPNGFile(circular_3_small_png), loadPNGFile(circular_3_png));
  sprite_space[EDITOR_SPRITE_BRUSH_CD_7x7] = newd EditorSprite(
      loadPNGFile(circular_4_small_png), loadPNGFile(circular_4_png));
  sprite_space[EDITOR_SPRITE_BRUSH_CD_9x9] = newd EditorSprite(
      loadPNGFile(circular_5_small_png), loadPNGFile(circular_5_png));
  sprite_space[EDITOR_SPRITE_BRUSH_CD_15x15] = newd EditorSprite(
      loadPNGFile(circular_6_small_png), loadPNGFile(circular_6_png));
  sprite_space[EDITOR_SPRITE_BRUSH_CD_19x19] = newd EditorSprite(
      loadPNGFile(circular_7_small_png), loadPNGFile(circular_7_png));
  sprite_space[EDITOR_SPRITE_BRUSH_SD_1x1] = newd EditorSprite(
      loadPNGFile(rectangular_1_small_png), loadPNGFile(rectangular_1_png));
  sprite_space[EDITOR_SPRITE_BRUSH_SD_3x3] = newd EditorSprite(
      loadPNGFile(rectangular_2_small_png), loadPNGFile(rectangular_2_png));
  sprite_space[EDITOR_SPRITE_BRUSH_SD_5x5] = newd EditorSprite(
      loadPNGFile(rectangular_3_small_png), loadPNGFile(rectangular_3_png));
  sprite_space[EDITOR_SPRITE_BRUSH_SD_7x7] = newd EditorSprite(
      loadPNGFile(rectangular_4_small_png), loadPNGFile(rectangular_4_png));
  sprite_space[EDITOR_SPRITE_BRUSH_SD_9x9] = newd EditorSprite(
      loadPNGFile(rectangular_5_small_png), loadPNGFile(rectangular_5_png));
  sprite_space[EDITOR_SPRITE_BRUSH_SD_15x15] = newd EditorSprite(
      loadPNGFile(rectangular_6_small_png), loadPNGFile(rectangular_6_png));
  sprite_space[EDITOR_SPRITE_BRUSH_SD_19x19] = newd EditorSprite(
      loadPNGFile(rectangular_7_small_png), loadPNGFile(rectangular_7_png));
  sprite_space[EDITOR_SPRITE_OPTIONAL_BORDER_TOOL] = newd EditorSprite(
      loadPNGFile(optional_border_small_png), loadPNGFile(optional_border_png));
  sprite_space[EDITOR_SPRITE_ERASER] =
      newd EditorSprite(loadPNGFile(eraser_small_png), loadPNGFile(eraser_png));
  sprite_space[EDITOR_SPRITE_PZ_TOOL] = newd EditorSprite(
      loadPNGFile(protection_zone_small_png), loadPNGFile(protection_zone_png));
  sprite_space[EDITOR_SPRITE_PVPZ_TOOL] = newd EditorSprite(
      loadPNGFile(pvp_zone_small_png), loadPNGFile(pvp_zone_png));
  sprite_space[EDITOR_SPRITE_NOLOG_TOOL] = newd EditorSprite(
      loadPNGFile(no_logout_small_png), loadPNGFile(no_logout_png));
  sprite_space[EDITOR_SPRITE_NOPVP_TOOL] =
      newd EditorSprite(loadPNGFile(no_pvp_small_png), loadPNGFile(no_pvp_png));

  {
    wxImage prefab_img;
    wxString path = "";
    if (wxFileExists("icons/prefab.png"))
      path = "icons/prefab.png";
    else if (wxFileExists("../icons/prefab.png"))
      path = "../icons/prefab.png";

    if (!path.IsEmpty() && prefab_img.LoadFile(path, wxBITMAP_TYPE_PNG)) {
      sprite_space[EDITOR_SPRITE_PREFAB] =
          newd EditorSprite(newd wxBitmap(prefab_img.Scale(16, 16)),
                            newd wxBitmap(prefab_img.Scale(32, 32)));
    } else {
      sprite_space[EDITOR_SPRITE_PREFAB] = newd EditorSprite(
          loadPNGFile(eraser_small_png), loadPNGFile(eraser_png));
    }
  }

  sprite_space[EDITOR_SPRITE_DOOR_NORMAL] = newd EditorSprite(
      loadPNGFile(door_normal_small_png), loadPNGFile(door_normal_png));
  sprite_space[EDITOR_SPRITE_DOOR_LOCKED] = newd EditorSprite(
      loadPNGFile(door_locked_small_png), loadPNGFile(door_locked_png));
  sprite_space[EDITOR_SPRITE_DOOR_MAGIC] = newd EditorSprite(
      loadPNGFile(door_magic_small_png), loadPNGFile(door_magic_png));
  sprite_space[EDITOR_SPRITE_DOOR_QUEST] = newd EditorSprite(
      loadPNGFile(door_quest_small_png), loadPNGFile(door_quest_png));

  wxBitmap *door_alt_small =
      LoadBitmapFromFile({"brushes/door_normal_alt_small.png",
                          "../brushes/door_normal_alt_small.png"});
  wxBitmap *door_alt = LoadBitmapFromFile(
      {"brushes/door_normal_alt.png", "../brushes/door_normal_alt.png"});
  sprite_space[EDITOR_SPRITE_DOOR_NORMAL_ALT] = newd EditorSprite(
      door_alt_small ? door_alt_small : loadPNGFile(door_normal_small_png),
      door_alt ? door_alt : loadPNGFile(door_normal_png));

  wxBitmap *door_archway_small = LoadBitmapFromFile(
      {"brushes/door_archway_small.png", "../brushes/door_archway_small.png"});
  wxBitmap *door_archway = LoadBitmapFromFile(
      {"brushes/door_archway.png", "../brushes/door_archway.png"});
  sprite_space[EDITOR_SPRITE_DOOR_ARCHWAY] = newd EditorSprite(
      door_archway_small ? door_archway_small
                         : loadPNGFile(door_normal_small_png),
      door_archway ? door_archway : loadPNGFile(door_normal_png));

  sprite_space[EDITOR_SPRITE_WINDOW_NORMAL] = newd EditorSprite(
      loadPNGFile(window_normal_small_png), loadPNGFile(window_normal_png));
  sprite_space[EDITOR_SPRITE_WINDOW_HATCH] = newd EditorSprite(
      loadPNGFile(window_hatch_small_png), loadPNGFile(window_hatch_png));
  sprite_space[EDITOR_SPRITE_SELECTION_GEM] =
      newd EditorSprite(loadPNGFile(gem_edit_png), nullptr);
  sprite_space[EDITOR_SPRITE_DRAWING_GEM] =
      newd EditorSprite(loadPNGFile(gem_move_png), nullptr);

  return true;
}

bool GraphicManager::loadOTFI(const FileName &filename, wxString &error,
                              wxArrayString &warnings) {
  wxDir dir(filename.GetFullPath());
  wxString otfi_file;
  otfi_found = false;

  if (dir.GetFirst(&otfi_file, "*.otfi", wxDIR_FILES)) {
    wxFileName otfi(filename.GetFullPath(), otfi_file);
    OTMLDocumentPtr doc = OTMLDocument::parse(otfi.GetFullPath().ToStdString());
    if (doc->size() == 0 || !doc->hasChildAt("DatSpr")) {
      error += "'DatSpr' tag not found";
      return false;
    }

    OTMLNodePtr node = doc->get("DatSpr");
    is_extended = node->valueAt<bool>("extended");
    has_transparency = node->valueAt<bool>("transparency");
    has_frame_durations = node->valueAt<bool>("frame-durations");
    has_frame_groups = node->valueAt<bool>("frame-groups");
    metadata_file =
        wxFileName(filename.GetFullPath(),
                   wxString(node->valueAt<std::string>(
                       "metadata-file", std::string(ASSETS_NAME) + ".dat")));
    sprites_file =
        wxFileName(filename.GetFullPath(),
                   wxString(node->valueAt<std::string>(
                       "sprites-file", std::string(ASSETS_NAME) + ".spr")));
    otfi_found = true;
  }

  if (!otfi_found) {
    is_extended = false;
    has_transparency = false;
    has_frame_durations = false;
    has_frame_groups = false;
    metadata_file =
        wxFileName(filename.GetFullPath(), wxString(ASSETS_NAME) + ".dat");
    sprites_file =
        wxFileName(filename.GetFullPath(), wxString(ASSETS_NAME) + ".spr");
  }
  return true;
}

bool GraphicManager::loadSpriteMetadata(const FileName &datafile,
                                        wxString &error,
                                        wxArrayString &warnings) {
  FileReadHandle file(nstr(datafile.GetFullPath()));
  if (!file.isOk()) {
    error += "Failed to open " + datafile.GetFullPath() + "\n" +
             wxstr(file.getErrorMessage());
    return false;
  }

  uint16_t effect_count, distance_count;
  uint32_t datSignature;
  file.getU32(datSignature);
  file.getU16(item_count);
  file.getU16(creature_count);
  file.getU16(effect_count);
  file.getU16(distance_count);

  uint32_t minID = 100;
  uint32_t maxID = item_count + creature_count;

  dat_format = client_version->getDatFormatForSignature(datSignature);
  if (dat_format == DAT_FORMAT_UNKNOWN)
    dat_format = DAT_FORMAT_1057;

  if (!otfi_found) {
    is_extended = dat_format >= DAT_FORMAT_96;
    has_frame_durations = dat_format >= DAT_FORMAT_1050;
    has_frame_groups = dat_format >= DAT_FORMAT_1057;
  }

  uint16_t id = minID;
  while (id <= maxID) {
    GameSprite *sType = newd GameSprite();
    sprite_space[id] = sType;
    sType->id = id;

    if (!loadSpriteMetadataFlags(file, sType, error, warnings)) {
      warnings.push_back(
          wxstr(std::format("Failed to load flags for sprite {}", sType->id)));
    }

    uint8_t group_count = 1;
    if (has_frame_groups && id > item_count)
      file.getU8(group_count);

    for (uint32_t k = 0; k < group_count; ++k) {
      if (has_frame_groups && id > item_count)
        file.skip(1);

      file.getByte(sType->width);
      file.getByte(sType->height);
      if ((sType->width > 1) || (sType->height > 1))
        file.skip(1);

      file.getU8(sType->layers);
      file.getU8(sType->pattern_x);
      file.getU8(sType->pattern_y);
      sType->pattern_z = (dat_format <= DAT_FORMAT_74)
                             ? 1
                             : (file.getU8(sType->pattern_z), sType->pattern_z);
      file.getU8(sType->frames);

      if (sType->frames > 1) {
        uint8_t async = 0;
        int loop_count = 0;
        int8_t start_frame = 0;
        if (has_frame_durations) {
          file.getByte(async);
          file.get32(loop_count);
          file.getSByte(start_frame);
        }
        sType->animator =
            newd Animator(sType->frames, start_frame, loop_count, async == 1);
        if (has_frame_durations) {
          for (int i = 0; i < sType->frames; i++) {
            uint32_t min, max;
            file.getU32(min);
            file.getU32(max);
            sType->animator->getFrameDuration(i)->setValues(int(min), int(max));
          }
          sType->animator->reset();
        }
      }

      sType->numsprites = (int)sType->width * (int)sType->height *
                          (int)sType->layers * (int)sType->pattern_x *
                          (int)sType->pattern_y * sType->pattern_z *
                          (int)sType->frames;

      for (uint32_t i = 0; i < sType->numsprites; ++i) {
        uint32_t sprite_id = 0;
        if (is_extended)
          file.getU32(sprite_id);
        else {
          uint16_t u16 = 0;
          file.getU16(u16);
          sprite_id = u16;
        }

        if (image_space[sprite_id] == nullptr) {
          GameSprite::NormalImage *img = newd GameSprite::NormalImage();
          img->id = sprite_id;
          image_space[sprite_id] = img;
        }
        sType->spriteList.push_back(
            static_cast<GameSprite::NormalImage *>(image_space[sprite_id]));
      }
    }
    ++id;
  }
  return true;
}

bool GraphicManager::loadSpriteMetadataFlags(FileReadHandle &file,
                                             GameSprite *sType, wxString &error,
                                             wxArrayString &warnings) {
  uint8_t prev_flag = 0;
  uint8_t flag = DatFlagLast;

  for (int i = 0; i < DatFlagLast; ++i) {
    prev_flag = flag;
    file.getU8(flag);
    if (flag == DatFlagLast)
      return true;

    if (dat_format >= DAT_FORMAT_1010) {
      if (flag == 16)
        flag = DatFlagNoMoveAnimation;
      else if (flag > 16)
        flag -= 1;
    } else if (dat_format >= DAT_FORMAT_78) {
      if (flag == 8)
        flag = DatFlagChargeable;
      else if (flag > 8)
        flag -= 1;
    } else if (dat_format >= DAT_FORMAT_755) {
      if (flag == 23)
        flag = DatFlagFloorChange;
    } else if (dat_format >= DAT_FORMAT_74) {
      if (flag > 0 && flag <= 15)
        flag += 1;
      else if (flag == 16)
        flag = DatFlagLight;
      else if (flag == 17)
        flag = DatFlagFloorChange;
      else if (flag == 18)
        flag = DatFlagFullGround;
      else if (flag == 19)
        flag = DatFlagElevation;
      else if (flag == 20)
        flag = DatFlagDisplacement;
      else if (flag == 22)
        flag = DatFlagMinimapColor;
      else if (flag == 23)
        flag = DatFlagRotateable;
      else if (flag == 24)
        flag = DatFlagLyingCorpse;
      else if (flag == 25)
        flag = DatFlagHangable;
      else if (flag == 26)
        flag = DatFlagHookSouth;
      else if (flag == 27)
        flag = DatFlagHookEast;
      else if (flag == 28)
        flag = DatFlagAnimateAlways;

      if (flag == DatFlagMultiUse)
        flag = DatFlagForceUse;
      else if (flag == DatFlagForceUse)
        flag = DatFlagMultiUse;
    }

    switch (flag) {
    case DatFlagGroundBorder:
    case DatFlagOnBottom:
    case DatFlagOnTop:
    case DatFlagContainer:
    case DatFlagStackable:
    case DatFlagForceUse:
    case DatFlagMultiUse:
    case DatFlagFluidContainer:
    case DatFlagSplash:
    case DatFlagNotWalkable:
    case DatFlagNotMoveable:
    case DatFlagBlockProjectile:
    case DatFlagNotPathable:
    case DatFlagPickupable:
    case DatFlagHangable:
    case DatFlagHookSouth:
    case DatFlagHookEast:
    case DatFlagRotateable:
    case DatFlagDontHide:
    case DatFlagTranslucent:
    case DatFlagLyingCorpse:
    case DatFlagAnimateAlways:
    case DatFlagFullGround:
    case DatFlagLook:
    case DatFlagWrappable:
    case DatFlagUnwrappable:
    case DatFlagTopEffect:
    case DatFlagFloorChange:
    case DatFlagNoMoveAnimation:
    case DatFlagChargeable:
      break;
    case DatFlagGround:
    case DatFlagWritable:
    case DatFlagWritableOnce:
    case DatFlagCloth:
    case DatFlagLensHelp:
    case DatFlagUsable:
      file.skip(2);
      break;
    case DatFlagLight: {
      uint16_t intensity, color;
      file.getU16(intensity);
      file.getU16(color);
      sType->has_light = true;
      sType->light = SpriteLight{static_cast<uint8_t>(intensity),
                                 static_cast<uint8_t>(color)};
      break;
    }
    case DatFlagDisplacement: {
      if (dat_format >= DAT_FORMAT_755) {
        file.getU16(sType->drawoffset_x);
        file.getU16(sType->drawoffset_y);
      } else {
        sType->drawoffset_x = 8;
        sType->drawoffset_y = 8;
      }
      break;
    }
    case DatFlagElevation:
      file.getU16(sType->draw_height);
      break;
    case DatFlagMinimapColor:
      file.getU16(sType->minimap_color);
      break;
    case DatFlagMarket: {
      file.skip(6);
      std::string marketName;
      file.getString(marketName);
      file.skip(4);
      break;
    }
    default:
      break;
    }
  }
  return true;
}

void GraphicManager::generateEmissiveLUT(uint32_t &textureId) {
  std::vector<float> lutData(4096 * 4, 0.0f);
  for (auto const &it : sprite_space) {
    int id = it.first;
    GameSprite *gs = dynamic_cast<GameSprite *>(it.second);
    if (gs && gs->has_light && id < 4096) {
      wxColor lightColor = colorFromEightBit(gs->light.color);
      float intensity = static_cast<float>(gs->light.intensity) / 255.0f;
      lutData[id * 4 + 0] = lightColor.Red() / 255.0f;
      lutData[id * 4 + 1] = lightColor.Green() / 255.0f;
      lutData[id * 4 + 2] = lightColor.Blue() / 255.0f;
      lutData[id * 4 + 3] = intensity;
    }
  }
  if (textureId == 0)
    glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 4096, 1, 0, GL_RGBA, GL_FLOAT,
               lutData.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);
}

bool GraphicManager::loadSpriteData(const FileName &datafile, wxString &error,
                                    wxArrayString &warnings) {
  FileReadHandle fh(nstr(datafile.GetFullPath()));
  if (!fh.isOk()) {
    error = "Failed to open file for reading";
    return false;
  }

  uint32_t sprSignature;
  if (!fh.getU32(sprSignature))
    return false;

  uint32_t total_pics = 0;
  if (is_extended)
    fh.getU32(total_pics);
  else {
    uint16_t u16 = 0;
    fh.getU16(u16);
    total_pics = u16;
  }

  if (!g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
    spritefile = nstr(datafile.GetFullPath());
    unloaded = false;
    return true;
  }

  std::vector<uint32_t> sprite_indexes;
  sprite_indexes.reserve(total_pics);
  for (uint32_t i = 0; i < total_pics; ++i) {
    uint32_t index;
    fh.getU32(index);
    sprite_indexes.push_back(index);
  }

  int id = 1;
  for (auto index_val : sprite_indexes) {
    uint32_t index = index_val + 3;
    fh.seek(index);
    uint16_t size;
    fh.getU16(size);

    auto it = image_space.find(id);
    if (it != image_space.end()) {
      GameSprite::NormalImage *spr =
          dynamic_cast<GameSprite::NormalImage *>(it->second);
      if (spr && size > 0) {
        if (spr->size == 0) {
          spr->id = id;
          spr->size = size;
          spr->dump = newd uint8_t[size];
          fh.getRAW(spr->dump, size);
        } else
          fh.seekRelative(size);
      }
    } else
      fh.seekRelative(size);
    ++id;
  }
  unloaded = false;
  return true;
}

bool GraphicManager::loadSpriteDump(uint8_t *&target, uint16_t &size,
                                    int sprite_id) {
  if (g_settings.getInteger(Config::USE_MEMCACHED_SPRITES) || sprite_id == 0) {
    size = 0;
    target = nullptr;
    return sprite_id == 0;
  }
  FileReadHandle fh(spritefile);
  if (!fh.isOk())
    return false;
  unloaded = false;

  if (!fh.seek((is_extended ? 4 : 2) + sprite_id * sizeof(uint32_t)))
    return false;

  uint32_t to_seek = 0;
  if (fh.getU32(to_seek)) {
    fh.seek(to_seek + 3);
    uint16_t sprite_size;
    if (fh.getU16(sprite_size)) {
      target = newd uint8_t[sprite_size];
      if (fh.getRAW(target, sprite_size)) {
        size = sprite_size;
        return true;
      }
      delete[] target;
      target = nullptr;
    }
  }
  return false;
}

void GraphicManager::addSpriteToCleanup(GameSprite *spr) {
  cleanup_list.push_back(spr);
  if (cleanup_list.size() >
      std::max<uint32_t>(
          100, g_settings.getInteger(Config::SOFTWARE_CLEAN_THRESHOLD))) {
    for (int i = 0; i < g_settings.getInteger(Config::SOFTWARE_CLEAN_SIZE) &&
                    !cleanup_list.empty();
         ++i) {
      cleanup_list.front()->unloadDC();
      cleanup_list.pop_front();
    }
  }
}

void GraphicManager::garbageCollection() {
  if (g_settings.getInteger(Config::TEXTURE_MANAGEMENT)) {
    int t = time(nullptr);
    if (loaded_textures >
            g_settings.getInteger(Config::TEXTURE_CLEAN_THRESHOLD) &&
        t - lastclean > g_settings.getInteger(Config::TEXTURE_CLEAN_PULSE)) {
      for (auto &iit : image_space)
        iit.second->clean(t);
      for (auto &sit : sprite_space) {
        if (GameSprite *gs = dynamic_cast<GameSprite *>(sit.second))
          gs->clean(t);
      }
      lastclean = t;
    }
  }
}

EditorSprite::EditorSprite(wxBitmap *b16x16, wxBitmap *b32x32) {
  bm[SPRITE_SIZE_16x16] = b16x16;
  bm[SPRITE_SIZE_32x32] = b32x32;
}

EditorSprite::~EditorSprite() {
  delete bm[SPRITE_SIZE_16x16];
  delete bm[SPRITE_SIZE_32x32];
}

void EditorSprite::DrawTo(wxDC *dc, SpriteSize sz, int start_x, int start_y,
                          int width, int height, bool count100) {
  wxBitmap *sp = bm[sz];
  if (sp) {
    int target_w = (width == -1) ? sp->GetWidth() : width;
    int target_h = (height == -1) ? sp->GetHeight() : height;
    if (sp->GetWidth() != target_w || sp->GetHeight() != target_h) {
      wxMemoryDC mem_dc(*sp);
      float scale = std::min((float)target_w / sp->GetWidth(),
                             (float)target_h / sp->GetHeight());
      int w = std::max(1, (int)(sp->GetWidth() * scale));
      int h = std::max(1, (int)(sp->GetHeight() * scale));
      dc->StretchBlit(start_x + (target_w - w) / 2,
                      start_y + (target_h - h) / 2, w, h, &mem_dc, 0, 0,
                      sp->GetWidth(), sp->GetHeight(), wxCOPY, true);
    } else
      dc->DrawBitmap(*sp, start_x, start_y, true);
  }
}

void EditorSprite::unloadDC() {
  delete bm[SPRITE_SIZE_16x16];
  delete bm[SPRITE_SIZE_32x32];
  bm[SPRITE_SIZE_16x16] = nullptr;
  bm[SPRITE_SIZE_32x32] = nullptr;
}

GameSprite::GameSprite()
    : id(0), height(0), width(0), layers(0), pattern_x(0), pattern_y(0),
      pattern_z(0), frames(0), numsprites(0), animator(nullptr), draw_height(0),
      drawoffset_x(0), drawoffset_y(0), minimap_color(0) {
  bm[SPRITE_SIZE_16x16] = nullptr;
  bm[SPRITE_SIZE_32x32] = nullptr;
  bm_100[SPRITE_SIZE_16x16] = nullptr;
  bm_100[SPRITE_SIZE_32x32] = nullptr;
}

GameSprite::~GameSprite() {
  delete bm[SPRITE_SIZE_16x16];
  delete bm[SPRITE_SIZE_32x32];
  delete bm_100[SPRITE_SIZE_16x16];
  delete bm_100[SPRITE_SIZE_32x32];
  for (auto *img : instanced_templates)
    delete img;
  delete animator;
}

void GameSprite::clean(int time) {
  for (auto *img : instanced_templates)
    img->clean(time);
}

int GameSprite::getDrawHeight() const { return draw_height; }
std::pair<int, int> GameSprite::getDrawOffset() const {
  return {drawoffset_x, drawoffset_y};
}
uint8_t GameSprite::getMiniMapColor() const { return minimap_color; }

int GameSprite::getIndex(int width_i, int height_i, int layer, int pattern_x_i,
                         int pattern_y_i, int pattern_z_i, int frame) const {
  int f = (this->frames > 0) ? (frame % static_cast<int>(this->frames)) : 0;
  int idx = f;
  idx = idx * static_cast<int>(this->pattern_z) + pattern_z_i;
  idx = idx * static_cast<int>(this->pattern_y) + pattern_y_i;
  idx = idx * static_cast<int>(this->pattern_x) + pattern_x_i;
  idx = idx * static_cast<int>(this->layers) + layer;
  idx = idx * static_cast<int>(this->height) + height_i;
  idx = idx * static_cast<int>(this->width) + width_i;
  return idx;
}

GLuint GameSprite::getHardwareID(int _x, int _y, int _layer, int _count,
                                 int _pattern_x, int _pattern_y, int _pattern_z,
                                 int _frame) {
  if (spriteList.empty() || numsprites == 0) return 0;
  uint32_t v =
      (_count >= 0 && height <= 1 && width <= 1)
          ? static_cast<uint32_t>(_count)
          : ((((((_frame)*pattern_y + _pattern_y) * pattern_x + _pattern_x) *
                   layers +
               _layer) *
                  height +
              _y) *
                 width +
             _x);
  if (v >= numsprites || v >= spriteList.size())
    v = (numsprites <= 1) ? 0 : (v % numsprites);
  if (v >= spriteList.size() || !spriteList[v]) return 0;
  return spriteList[v]->getHardwareID();
}

// OPTIMIZATION: Linear search replaced with fast direct lookup cache for
// instanced outfit templates
GameSprite::TemplateImage *GameSprite::getTemplateImage(int sprite_index,
                                                        const Outfit &outfit) {
  uint32_t lookHash = outfit.getColorHash();
  for (auto *img : instanced_templates) {
    if (img->sprite_index == sprite_index) {
      uint32_t curHash = (img->lookHead << 24) | (img->lookBody << 16) |
                         (img->lookLegs << 8) | img->lookFeet;
      if (curHash == lookHash)
        return img;
    }
  }
  TemplateImage *img = newd TemplateImage(this, sprite_index, outfit);
  instanced_templates.push_back(img);
  return img;
}

GLuint GameSprite::getHardwareID(int _x, int _y, int _dir, int _addon,
                                 int _pattern_z, const Outfit &_outfit,
                                 int _frame) {
  if (spriteList.empty() || numsprites == 0) return 0;
  uint32_t v = getIndex(_x, _y, 0, _dir, _addon, _pattern_z, _frame);
  if (v >= numsprites || v >= spriteList.size())
    v = (numsprites <= 1) ? 0 : (v % numsprites);
  if (layers > 1)
    return getTemplateImage(v, _outfit)->getHardwareID();
  if (v >= spriteList.size() || !spriteList[v]) return 0;
  return spriteList[v]->getHardwareID();
}

wxBitmap *GameSprite::getBitmap(SpriteSize size, bool count100) {
  wxBitmap *&target_bm = count100 ? bm_100[size] : bm[size];
  if (!target_bm) {
    const int bgshade = g_settings.getInteger(Config::ICON_BACKGROUND);
    int image_size = std::max<int>(width, height) * SPRITE_PIXELS;
    wxImage image(image_size, image_size);
    image.Clear(bgshade);

    int px = 0, py = 0;
    if (count100) {
      px = std::min(7 % std::max<int>(1, pattern_x), (int)pattern_x - 1);
      py = std::min((7 / std::max<int>(1, pattern_x)) %
                        std::max<int>(1, pattern_y),
                    (int)pattern_y - 1);
    }

    for (uint8_t l = 0; l < layers; l++) {
      for (uint8_t w = 0; w < width; w++) {
        for (uint8_t h = 0; h < height; h++) {
          const int i = getIndex(w, h, l, px, py, 0, 0);
          if (i >= 0 && (size_t)i < spriteList.size() && spriteList[i]) {
            uint8_t *data = spriteList[i]->getRGBData();
            if (data) {
              wxImage img(SPRITE_PIXELS, SPRITE_PIXELS, data, true);
              img.SetMaskColour(0xFF, 0x00, 0xFF);
              image.Paste(img, (width - w - 1) * SPRITE_PIXELS,
                          (height - h - 1) * SPRITE_PIXELS);
              img.Destroy();
              delete[] data;
            }
          }
        }
      }
    }

    if (size == SPRITE_SIZE_16x16 || image.GetWidth() > SPRITE_PIXELS ||
        image.GetHeight() > SPRITE_PIXELS) {
      int new_size = (size == SPRITE_SIZE_16x16) ? 16 : 32;
      image.Rescale(new_size, new_size);
    }

    target_bm = newd wxBitmap(image);
    g_gui.gfx.addSpriteToCleanup(this);
    image.Destroy();
  }
  return target_bm;
}

void GameSprite::DrawTo(wxDC *dc, SpriteSize sz, int start_x, int start_y,
                        int width_val, int height_val, bool count100) {
  if (width_val == -1)
    width_val = (sz == SPRITE_SIZE_32x32) ? 32 : 16;
  if (height_val == -1)
    height_val = (sz == SPRITE_SIZE_32x32) ? 32 : 16;
  wxBitmap *sdc = getBitmap(sz, count100);
  if (sdc) {
    int bmp_w = sdc->GetWidth(), bmp_h = sdc->GetHeight();
    if (bmp_w != width_val || bmp_h != height_val) {
      wxMemoryDC mem_dc(*sdc);
      float scale =
          std::min((float)width_val / bmp_w, (float)height_val / bmp_h);
      int w = std::max(1, (int)(bmp_w * scale)),
          h = std::max(1, (int)(bmp_h * scale));
      dc->StretchBlit(start_x + (width_val - w) / 2,
                      start_y + (height_val - h) / 2, w, h, &mem_dc, 0, 0,
                      bmp_w, bmp_h, wxCOPY, true);
    } else
      dc->DrawBitmap(*sdc, start_x, start_y, true);
  } else {
    const wxBrush &b = dc->GetBrush();
    dc->SetBrush(*wxRED_BRUSH);
    dc->DrawRectangle(start_x, start_y, width_val, height_val);
    dc->SetBrush(b);
  }
}

void GameSprite::DrawOutfitTo(wxDC *dc, const Outfit &outfit, int start_x,
                               int start_y, int width_val, int height_val,
                               int dir, int addon, int pattern_z, int frame) {
  if (!dc)
    return;

  int grid_w = std::max<int>(1, (int)width);
  int grid_h = std::max<int>(1, (int)height);
  int total_px_w = grid_w * 32;
  int total_px_h = grid_h * 32;

  wxImage composite(total_px_w, total_px_h, true);
  composite.InitAlpha();
  unsigned char* alpha_buf = composite.GetAlpha();
  if (alpha_buf) {
    memset(alpha_buf, 0, total_px_w * total_px_h);
  }

  bool drawn_any = false;

  for (uint8_t w = 0; w < width; ++w) {
    for (uint8_t h = 0; h < height; ++h) {
      int v = getIndex(w, h, 0, dir, addon, pattern_z, frame);
      uint8_t *rgba = nullptr;
      if (layers >= 2) {
        TemplateImage *timg = getTemplateImage(v, outfit);
        if (timg)
          rgba = timg->getRGBAData();
      } else if (v < (int)numsprites && spriteList[v]) {
        rgba = spriteList[v]->getRGBAData();
      }

      if (rgba) {
        wxImage tile_img(32, 32, false);
        unsigned char *rgb = (unsigned char *)malloc(32 * 32 * 3);
        unsigned char *alpha = (unsigned char *)malloc(32 * 32);
        for (int i = 0; i < 32 * 32; ++i) {
          rgb[i * 3 + 0] = rgba[i * 4 + 0];
          rgb[i * 3 + 1] = rgba[i * 4 + 1];
          rgb[i * 3 + 2] = rgba[i * 4 + 2];
          alpha[i] = rgba[i * 4 + 3];
        }
        tile_img.SetData(rgb);
        tile_img.SetAlpha(alpha);
        delete[] rgba;

        int dest_x = (width - w - 1) * 32;
        int dest_y = (height - h - 1) * 32;
        composite.Paste(tile_img, dest_x, dest_y);
        tile_img.Destroy();
        drawn_any = true;
      }
    }
  }

  if (drawn_any) {
    float scale = std::min((float)width_val / total_px_w, (float)height_val / total_px_h);
    int final_w = std::max(1, (int)(total_px_w * scale));
    int final_h = std::max(1, (int)(total_px_h * scale));
    int offset_x = start_x + (width_val - final_w) / 2;
    int offset_y = start_y + (height_val - final_h) / 2;

    wxBitmap bmp(composite.Scale(final_w, final_h, wxIMAGE_QUALITY_NEAREST));
    dc->DrawBitmap(bmp, offset_x, offset_y, true);
    composite.Destroy();
  } else {
    DrawTo(dc, SPRITE_SIZE_32x32, start_x, start_y, width_val, height_val);
  }
}

void GameSprite::unloadDC() {
  delete bm[SPRITE_SIZE_16x16];
  delete bm[SPRITE_SIZE_32x32];
  bm[SPRITE_SIZE_16x16] = nullptr;
  bm[SPRITE_SIZE_32x32] = nullptr;
  delete bm_100[SPRITE_SIZE_16x16];
  delete bm_100[SPRITE_SIZE_32x32];
  bm_100[SPRITE_SIZE_16x16] = nullptr;
  bm_100[SPRITE_SIZE_32x32] = nullptr;
}

GameSprite::Image::Image()
    : isGLLoaded(false), isGLQueueing(false), lastaccess(0) {}
GameSprite::Image::~Image() { unloadGLTexture(0); }

void GameSprite::Image::createGLTexture(GLuint whatid) {
  uint8_t *rgba = getRGBAData();
  if (!rgba)
    return;

  isGLLoaded = true;
  g_gui.gfx.loaded_textures += 1;

  glBindTexture(GL_TEXTURE_2D, whatid);
  GLint filter = g_settings.getBoolean(Config::FAKE_HD_ASSETS) ? GL_LINEAR : GL_NEAREST;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SPRITE_PIXELS, SPRITE_PIXELS, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  delete[] rgba;
}

void GameSprite::Image::unloadGLTexture(GLuint whatid) {
  isGLLoaded = false;
  g_gui.gfx.loaded_textures -= 1;
  if (whatid != 0)
    glDeleteTextures(1, &whatid);
}

void GameSprite::Image::visit() { lastaccess = time(nullptr); }

void GameSprite::Image::clean(int time_val) {
  if (isGLLoaded &&
      time_val - lastaccess > g_settings.getInteger(Config::TEXTURE_LONGEVITY))
    unloadGLTexture(0);
}

GameSprite::NormalImage::NormalImage() : id(0), size(0), dump(nullptr) {}
GameSprite::NormalImage::~NormalImage() { delete[] dump; }

void GameSprite::NormalImage::clean(int time_val) {
  Image::clean(time_val);
  if (time_val - lastaccess > 5 &&
      !g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
    delete[] dump;
    dump = nullptr;
  }
}

uint8_t *GameSprite::NormalImage::getRGBData() {
  if (!dump && !g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
    if (!g_gui.gfx.loadSpriteDump(dump, size, id))
      return nullptr;
  }
  if (!dump)
    return nullptr;

  const int pixels_data_size = SPRITE_PIXELS * SPRITE_PIXELS * 3;
  uint8_t *data = newd uint8_t[pixels_data_size];
  uint8_t bpp = g_gui.gfx.hasTransparency() ? 4 : 3;
  int write = 0, read = 0;

  while (read < size && write < pixels_data_size) {
    int transparent = dump[read] | (dump[read + 1] << 8);
    read += 2;
    for (int i = 0; i < transparent && write < pixels_data_size; i++) {
      data[write + 0] = 0xFF;
      data[write + 1] = 0x00;
      data[write + 2] = 0xFF;
      write += 3;
    }
    int colored = dump[read] | (dump[read + 1] << 8);
    read += 2;
    for (int i = 0; i < colored && write < pixels_data_size; i++) {
      data[write + 0] = dump[read + 0];
      data[write + 1] = dump[read + 1];
      data[write + 2] = dump[read + 2];
      write += 3;
      read += bpp;
    }
  }
  while (write < pixels_data_size) {
    data[write + 0] = 0xFF;
    data[write + 1] = 0x00;
    data[write + 2] = 0xFF;
    write += 3;
  }
  return data;
}

uint8_t *GameSprite::NormalImage::getRGBAData() {
  if (!dump && !g_settings.getInteger(Config::USE_MEMCACHED_SPRITES)) {
    if (!g_gui.gfx.loadSpriteDump(dump, size, id))
      return nullptr;
  }
  if (!dump)
    return nullptr;

  const int pixels_data_size = SPRITE_PIXELS_SIZE * 4;
  uint8_t *data = newd uint8_t[pixels_data_size];
  bool use_alpha = g_gui.gfx.hasTransparency();
  uint8_t bpp = use_alpha ? 4 : 3;
  int write = 0, read = 0;

  while (read < size && write < pixels_data_size) {
    int transparent = dump[read] | (dump[read + 1] << 8);
    if (use_alpha && transparent >= SPRITE_PIXELS_SIZE)
      break;
    read += 2;
    for (int i = 0; i < transparent && write < pixels_data_size; i++) {
      data[write + 0] = 0;
      data[write + 1] = 0;
      data[write + 2] = 0;
      data[write + 3] = 0;
      write += 4;
    }
    int colored = dump[read] | (dump[read + 1] << 8);
    read += 2;
    for (int i = 0; i < colored && write < pixels_data_size; i++) {
      data[write + 0] = dump[read + 0];
      data[write + 1] = dump[read + 1];
      data[write + 2] = dump[read + 2];
      data[write + 3] = use_alpha ? dump[read + 3] : 0xFF;
      write += 4;
      read += bpp;
    }
  }
  while (write < pixels_data_size) {
    data[write + 0] = 0;
    data[write + 1] = 0;
    data[write + 2] = 0;
    data[write + 3] = 0;
    write += 4;
  }
  return data;
}

GLuint GameSprite::NormalImage::getHardwareID() {
  if (!isGLLoaded) {
    if (!isGLQueueing) {
      isGLQueueing = true;
      if (g_gui.async_loader) {
        g_gui.async_loader->queueSpriteLoad(id, "");
        this->isGLQueueing = false;
        if (this->dump)
          this->createGLTexture(this->id);
      } else {
        createGLTexture(id);
        isGLQueueing = false;
        return id;
      }
    }
    return 0;
  }
  visit();
  return id;
}

void GameSprite::NormalImage::createGLTexture(GLuint ignored) {
  Image::createGLTexture(id);
}
void GameSprite::NormalImage::unloadGLTexture(GLuint ignored) {
  Image::unloadGLTexture(id);
}

GameSprite::TemplateImage::TemplateImage(GameSprite *parent, int v,
                                         const Outfit &outfit)
    : gl_tid(0), parent(parent), sprite_index(v), lookHead(outfit.lookHead),
      lookBody(outfit.lookBody), lookLegs(outfit.lookLegs),
      lookFeet(outfit.lookFeet) {}

GameSprite::TemplateImage::~TemplateImage() {}

void GameSprite::TemplateImage::colorizePixel(uint8_t color, uint8_t &red,
                                              uint8_t &green, uint8_t &blue) {
  uint8_t ro = (TemplateOutfitLookupTable[color] & 0xFF0000) >> 16;
  uint8_t go = (TemplateOutfitLookupTable[color] & 0xFF00) >> 8;
  uint8_t bo = (TemplateOutfitLookupTable[color] & 0xFF);
  red = (uint8_t)(red * (ro / 255.f));
  green = (uint8_t)(green * (go / 255.f));
  blue = (uint8_t)(blue * (bo / 255.f));
}

uint8_t *GameSprite::TemplateImage::getRGBData() {
  uint8_t *rgbdata = parent->spriteList[sprite_index]->getRGBData();
  uint8_t *template_rgbdata =
      parent->spriteList[sprite_index + parent->height * parent->width]
          ->getRGBData();
  if (!rgbdata || !template_rgbdata) {
    delete[] rgbdata;
    delete[] template_rgbdata;
    return nullptr;
  }

  size_t lutSize =
      sizeof(TemplateOutfitLookupTable) / sizeof(TemplateOutfitLookupTable[0]);
  if (lookHead >= lutSize)
    lookHead = 0;
  if (lookBody >= lutSize)
    lookBody = 0;
  if (lookLegs >= lutSize)
    lookLegs = 0;
  if (lookFeet >= lutSize)
    lookFeet = 0;

  for (int y = 0; y < SPRITE_PIXELS; ++y) {
    for (int x = 0; x < SPRITE_PIXELS; ++x) {
      int idx = (y * SPRITE_PIXELS + x) * 3;
      uint8_t &red = rgbdata[idx + 0];
      uint8_t &green = rgbdata[idx + 1];
      uint8_t &blue = rgbdata[idx + 2];
      uint8_t tred = template_rgbdata[idx + 0],
              tgreen = template_rgbdata[idx + 1],
              tblue = template_rgbdata[idx + 2];

      if (tred && tgreen && !tblue)
        colorizePixel(lookHead, red, green, blue);
      else if (tred && !tgreen && !tblue)
        colorizePixel(lookBody, red, green, blue);
      else if (!tred && tgreen && !tblue)
        colorizePixel(lookLegs, red, green, blue);
      else if (!tred && !tgreen && tblue)
        colorizePixel(lookFeet, red, green, blue);
    }
  }
  delete[] template_rgbdata;
  return rgbdata;
}

uint8_t *GameSprite::TemplateImage::getRGBAData() {
  uint8_t *rgbadata = parent->spriteList[sprite_index]->getRGBAData();
  uint8_t *template_rgbdata =
      parent->spriteList[sprite_index + parent->height * parent->width]
          ->getRGBData();
  if (!rgbadata || !template_rgbdata) {
    delete[] rgbadata;
    delete[] template_rgbdata;
    return nullptr;
  }

  size_t lutSize =
      sizeof(TemplateOutfitLookupTable) / sizeof(TemplateOutfitLookupTable[0]);
  if (lookHead >= lutSize)
    lookHead = 0;
  if (lookBody >= lutSize)
    lookBody = 0;
  if (lookLegs >= lutSize)
    lookLegs = 0;
  if (lookFeet >= lutSize)
    lookFeet = 0;

  for (int y = 0; y < SPRITE_PIXELS; ++y) {
    for (int x = 0; x < SPRITE_PIXELS; ++x) {
      int idx4 = (y * SPRITE_PIXELS + x) * 4;
      int idx3 = (y * SPRITE_PIXELS + x) * 3;
      uint8_t &red = rgbadata[idx4 + 0];
      uint8_t &green = rgbadata[idx4 + 1];
      uint8_t &blue = rgbadata[idx4 + 2];
      uint8_t tred = template_rgbdata[idx3 + 0],
              tgreen = template_rgbdata[idx3 + 1],
              tblue = template_rgbdata[idx3 + 2];

      if (tred && tgreen && !tblue)
        colorizePixel(lookHead, red, green, blue);
      else if (tred && !tgreen && !tblue)
        colorizePixel(lookBody, red, green, blue);
      else if (!tred && tgreen && !tblue)
        colorizePixel(lookLegs, red, green, blue);
      else if (!tred && !tgreen && tblue)
        colorizePixel(lookFeet, red, green, blue);
    }
  }
  delete[] template_rgbdata;
  return rgbadata;
}

GLuint GameSprite::TemplateImage::getHardwareID() {
  if (!isGLLoaded) {
    if (gl_tid == 0)
      gl_tid = g_gui.gfx.getFreeTextureID();
    createGLTexture(gl_tid);
    if (!isGLLoaded)
      return 0;
  }
  visit();
  return gl_tid;
}

void GameSprite::TemplateImage::createGLTexture(GLuint unused) {
  Image::createGLTexture(gl_tid);
}
void GameSprite::TemplateImage::unloadGLTexture(GLuint unused) {
  Image::unloadGLTexture(gl_tid);
}

Animator::Animator(int frame_count, int start_frame, int loop_count, bool async)
    : frame_count(frame_count), start_frame(start_frame),
      loop_count(loop_count), async(async), current_frame(0), current_loop(0),
      current_duration(0), total_duration(0), direction(ANIMATION_FORWARD),
      last_time(0), is_complete(false) {
  durations.reserve(frame_count);
  for (int i = 0; i < frame_count; i++)
    durations.push_back(
        newd FrameDuration(ITEM_FRAME_DURATION, ITEM_FRAME_DURATION));
  reset();
}

Animator::~Animator() {
  for (auto *fd : durations)
    delete fd;
  durations.clear();
}

int Animator::getStartFrame() const {
  return (start_frame > -1) ? start_frame : uniform_random(0, frame_count - 1);
}

FrameDuration *Animator::getFrameDuration(int frame) {
  return durations[frame];
}

int Animator::getFrame() {
  if (!async && total_duration > 0) {
    calculateSynchronous();
    return current_frame;
  }

  long time_val = g_gui.gfx.getElapsedTime();
  if (time_val != last_time && !is_complete) {
    long elapsed = time_val - last_time;
    if (elapsed >= current_duration) {
      int frame = (loop_count < 0) ? getPingPongFrame() : getLoopFrame();
      if (current_frame != frame) {
        int duration = getDuration(frame) - (elapsed - current_duration);
        if (duration < 0 && !async)
          calculateSynchronous();
        else {
          current_frame = frame;
          current_duration = std::max<int>(0, duration);
        }
      } else if (loop_count != 0) {
        is_complete = true;
      }
    } else
      current_duration -= elapsed;
    last_time = time_val;
  }
  return current_frame;
}

void Animator::setFrame(int frame) {
  if (current_frame == frame)
    return;
  if (async) {
    if (frame == 255)
      current_frame = 0;
    else if (frame == 254)
      current_frame = uniform_random(0, frame_count - 1);
    else if (frame >= 0 && frame < frame_count)
      current_frame = frame;
    else
      current_frame = getStartFrame();

    is_complete = false;
    last_time = g_gui.gfx.getElapsedTime();
    current_duration = getDuration(current_frame);
    current_loop = 0;
  } else
    calculateSynchronous();
}

void Animator::reset() {
  total_duration = 0;
  for (int i = 0; i < frame_count; i++)
    total_duration += durations[i]->max;
  is_complete = false;
  direction = ANIMATION_FORWARD;
  current_loop = 0;
  async = false;
  setFrame(-1);
}

int Animator::getDuration(int frame) const {
  return durations[frame]->getDuration();
}

int Animator::getPingPongFrame() {
  int count = (direction == ANIMATION_FORWARD) ? 1 : -1;
  int next_frame = current_frame + count;
  if (next_frame < 0 || next_frame >= frame_count) {
    direction = (direction == ANIMATION_FORWARD) ? ANIMATION_BACKWARD
                                                 : ANIMATION_FORWARD;
    count *= -1;
  }
  return current_frame + count;
}

int Animator::getLoopFrame() {
  int next_phase = current_frame + 1;
  if (next_phase < frame_count)
    return next_phase;
  if (loop_count == 0)
    return 0;
  if (current_loop < (loop_count - 1)) {
    current_loop++;
    return 0;
  }
  return current_frame;
}

void Animator::calculateSynchronous() {
  long time_val = g_gui.gfx.getElapsedTime();
  if (time_val > 0 && total_duration > 0) {
    long elapsed = time_val % total_duration;
    int total_time = 0;
    for (int i = 0; i < frame_count; i++) {
      int duration = getDuration(i);
      if (elapsed >= total_time && elapsed < total_time + duration) {
        current_frame = i;
        current_duration = duration - (elapsed - total_time);
        break;
      }
      total_time += duration;
    }
    last_time = time_val;
  }
}