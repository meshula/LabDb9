#include "MemexVerbs.h"
// Phase 1: Chunk Navigation Verbs (Implemented)
#include "Memex/MemexChunkPrecedingVerb.h"
#include "Memex/MemexChunkSucceedingVerb.h"

// Phase 1: Chunk Extension Verbs (TODO)
// #include "Memex/MemexChunkExtendVerb.h"
// #include "Memex/MemexChunkExtendSemanticVerb.h"

// Phase 2: Trail Management Verbs (TODO)
// #include "Memex/MemexTrailCreateVerb.h"
// #include "Memex/MemexTrailNavigateVerb.h"
// #include "Memex/MemexTrailSaveVerb.h"
// #include "Memex/MemexTrailLoadVerb.h"
#include "LabDb/Db9Dispatcher.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace LabDb {

//-----------------------------------------------------------------------------
// Registration Function
//-----------------------------------------------------------------------------

void initMemexVerbRegistration(Db9Dispatcher& dispatcher) {
    static bool registered = false;
    if (!registered) {
        // Phase 1: Chunk Navigation Verbs (Implemented)
        dispatcher.registerVerb(std::make_unique<MemexChunkPrecedingVerb>());
        dispatcher.registerVerb(std::make_unique<MemexChunkSucceedingVerb>());
        
        // Phase 1: Chunk Extension Verbs (TODO: Implement next)
        // dispatcher.registerVerb(std::make_unique<MemexChunkExtendVerb>());
        // dispatcher.registerVerb(std::make_unique<MemexChunkExtendSemanticVerb>());
        
        // Phase 2: Trail Management Verbs (TODO: Implement later)
        // dispatcher.registerVerb(std::make_unique<MemexTrailCreateVerb>());
        // dispatcher.registerVerb(std::make_unique<MemexTrailNavigateVerb>());
        // dispatcher.registerVerb(std::make_unique<MemexTrailSaveVerb>());
        // dispatcher.registerVerb(std::make_unique<MemexTrailLoadVerb>());
        
        registered = true;
    }
}

} // namespace LabDb
