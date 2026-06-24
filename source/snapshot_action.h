#ifndef RME_SNAPSHOT_ACTION_H
#define RME_SNAPSHOT_ACTION_H

#include "action.h"
#include "map.h"
#include <vector>

namespace RME::Core {

class SnapshotAction : public Action {
public:
    SnapshotAction(Editor& editor, Map& map, int x, int y, int w, int h) 
        : Action(editor, ACTION_SNAPSHOT), m_map(map), m_x(x), m_y(y), m_w(w), m_h(h) {
        m_before = m_map.createRegionSnapshot(m_x, m_y, m_w, m_h);
    }

    void captureAfter() {
        m_after = m_map.createRegionSnapshot(m_x, m_y, m_w, m_h);
    }

    virtual void undo(DirtyList* dirty_list) override {
        m_map.restoreRegionSnapshot(m_x, m_y, m_before);
    }

    virtual void commit(DirtyList* dirty_list) override {
        m_map.restoreRegionSnapshot(m_x, m_y, m_after);
    }

    virtual size_t size() const override {
        return 1;
    }

    virtual size_t memsize() const override {
        return sizeof(*this) + m_before.size() + m_after.size();
    }

    virtual size_t approx_memsize() const override {
        return memsize();
    }

private:
    Map& m_map;
    int m_x, m_y, m_w, m_h;
    std::vector<uint8_t> m_before;
    std::vector<uint8_t> m_after;
};

} // namespace RME::Core

#endif