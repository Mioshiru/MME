#include "main.h"
#include "iomap_otmm.h"

// OTMM support is currently not used by the active build path. This file is kept
// intentionally minimal to avoid conflicting with the existing OTBM-based code.

namespace {

class OTMMStubMapHandler : public IOMapOTMM {
public:
	bool loadMap(Map&, const FileName&, bool) override { return true; }
	bool loadMap(Map&, NodeFileReadHandle&, const FileName&, bool) override { return true; }
	bool saveMap(Map&, const FileName&, bool) override { return true; }
	bool saveMap(Map&, NodeFileWriteHandle&, const FileName&, bool) override { return true; }
};

} // namespace

IOMapOTMM::IOMapOTMM() = default;
IOMapOTMM::~IOMapOTMM() = default;

MapVersion IOMapOTMM::getVersionInfo(const FileName&) {
	return MapVersion(MAP_OTBM_UNKNOWN, CLIENT_VERSION_NONE);
}

bool IOMapOTMM::loadMap(Map&, const FileName&, bool) {
	return true;
}

bool IOMapOTMM::loadMap(Map&, NodeFileReadHandle&, const FileName&, bool) {
	return true;
}

bool IOMapOTMM::saveMap(Map&, const FileName&, bool) {
	return true;
}

bool IOMapOTMM::saveMap(Map&, NodeFileWriteHandle&, const FileName&, bool) {
	return true;
}
