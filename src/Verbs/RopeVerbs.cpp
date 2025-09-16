#include "RopeVerbs.h"
#include "Ropes/RopeCreateVerb.h"
#include "Ropes/RopeAppendVerb.h"
#include "Ropes/RopeTraverseVerb.h"
#include "Ropes/RopeChunkVerb.h"
#include "Ropes/RopeListVerb.h"
#include "Ropes/RopeInfoVerb.h"
#include "Ropes/RopeDeleteVerb.h"
#include "Ropes/RopeInsertVerb.h"
#include "Ropes/RopeFindVerb.h"
#include "Ropes/RopeUtils.h"
#include "LabDb/Db9Dispatcher.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace LabDb {

//-----------------------------------------------------------------------------
// Registration Function
//-----------------------------------------------------------------------------

void initRopeVerbRegistration(Db9Dispatcher& dispatcher) {
    static bool registered = false;
    if (!registered) {
        // Core rope operations
        dispatcher.registerVerb(std::make_unique<RopeCreateVerb>());
        dispatcher.registerVerb(std::make_unique<RopeAppendVerb>());
        dispatcher.registerVerb(std::make_unique<RopeTraverseVerb>());
        
        // Memex navigation operations
        dispatcher.registerVerb(std::make_unique<RopeChunkVerb>());
        dispatcher.registerVerb(std::make_unique<RopeFindVerb>());
        dispatcher.registerVerb(std::make_unique<RopeInsertVerb>());
        
        // Management operations
        dispatcher.registerVerb(std::make_unique<RopeListVerb>());
        dispatcher.registerVerb(std::make_unique<RopeInfoVerb>());
        dispatcher.registerVerb(std::make_unique<RopeDeleteVerb>());
        
        registered = true;
    }
}

} // namespace LabDb
