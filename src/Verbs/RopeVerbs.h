#pragma once

#include "LabDb/Verbs.h"
#include "LabDb/Db9Response.h"
#include "LabDb/LabText.hpp"
#include <string>
#include <vector>
#include <memory>

namespace LabDb {

class Db9Dispatcher;

//-----------------------------------------------------------------------------
// Forward Declarations for Rope Verbs
//-----------------------------------------------------------------------------

class RopeCreateVerb;
class RopeAppendVerb;
class RopeTraverseVerb;
class RopeInfoVerb;
class RopeListVerb;
class RopeChunkVerb;
class RopeDeleteVerb;
class RopeInsertVerb;
class RopeFindVerb;

//-----------------------------------------------------------------------------
// Registration Function
//-----------------------------------------------------------------------------

/// Initialize and register all Rope verbs with the dispatcher
void initRopeVerbRegistration(Db9Dispatcher& dispatcher);

} // namespace LabDb