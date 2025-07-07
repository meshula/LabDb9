#include "LabDb/EntityId.h"
#include "LabDb/NonoStore.h"

namespace LabDb {

EntityId EntityId::fromName(const std::string& name, const NonoStore& store) {
    // Use SynchronousTransaction for clean encapsulation
    auto sync = const_cast<NonoStore&>(store).begin_sync();
    if (!sync) {
        return EntityId(name, "", INVALID_TID, false);
    }
    
    return sync->intern_eid(name);
}

EntityId EntityId::fromEid(const std::string& eid, const NonoStore& store) {
    // Use SynchronousTransaction for clean encapsulation
    auto sync = const_cast<NonoStore&>(store).begin_sync();
    if (!sync) {
        return EntityId("", eid, INVALID_TID, false);
    }
    
    return sync->resolve_eid(eid);
}

EntityId EntityId::fromTid(const TID& tid, const NonoStore& store) {
    // Use SynchronousTransaction for clean encapsulation
    auto sync = const_cast<NonoStore&>(store).begin_sync();
    if (!sync) {
        return EntityId("", "", tid, false);
    }
    
    return sync->resolve_tid(tid);
}

} // namespace LabDb
