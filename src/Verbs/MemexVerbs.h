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
// Forward Declarations for Memex Verbs
//-----------------------------------------------------------------------------

class MemexChunkPrecedingVerb;
class MemexChunkSucceedingVerb;
class MemexChunkExtendVerb;
class MemexChunkExtendSemanticVerb;
class MemexTrailCreateVerb;
class MemexTrailNavigateVerb;
class MemexTrailSaveVerb;
class MemexTrailLoadVerb;

//-----------------------------------------------------------------------------
// Registration Function
//-----------------------------------------------------------------------------

/// Initialize and register all Memex verbs with the dispatcher
void initMemexVerbRegistration(Db9Dispatcher& dispatcher);

} // namespace LabDb
