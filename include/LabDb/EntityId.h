#pragma once

#include <string>
#include <cstddef>
#include <sstream>

namespace LabDb {

// Forward declarations
class NonoStore;
using TID = uint64_t;
static constexpr TID INVALID_TID = 0;

//-----------------------------------------------------------------------------
// EntityId - Enhanced Implementation with NonoStore Integration
// 
// Integrates with NonoStore's TermDictionary for proper EID→TID→name mapping
// through existing authority structure and separation of concerns.
//
// Based on entityid-interface-design.md specification
//-----------------------------------------------------------------------------

class EntityId {
public:
    // Factory methods for different construction patterns
    static EntityId fromName(const std::string& name, const NonoStore& store);
    static EntityId fromEid(const std::string& eid, const NonoStore& store);
    static EntityId fromTid(const TID& tid, const NonoStore& store);
    
    // Minimal factory for backward compatibility (no store access)
    static EntityId fromEid(const std::string& eid) {
        return EntityId("", eid, parseEidNumeric(eid), false);
    }
    
    explicit EntityId(const std::string& name, const std::string& eid,
                     const TID& tid, bool exists)
        : _name(name), _eid(eid), _tid(tid), _exists(exists) {}

    
    // Query interface
    const std::string& name() const { return _name; }
    const std::string& eid() const { return _eid; }
    const TID& tid() const { return _tid; }
    
    // State queries
    bool exists() const { return _exists; }
    bool isValid() const { return _tid != INVALID_TID; }
    
    // Equality and comparison (based on TID for proper semantics)
    bool operator==(const EntityId& other) const { return _tid == other._tid; }
    bool operator!=(const EntityId& other) const { return _tid != other._tid; }
    bool operator<(const EntityId& other) const { return _tid < other._tid; }
    
    // String conversion for S-expression compatibility
    std::string toEidString() const { return _eid; }
    std::string toNameString() const { return _name; }
    
    // Legacy compatibility for current GetEntityVerb
    size_t index() const { return static_cast<size_t>(_tid); }
    
    // Static helper methods for SynchronousTransaction
    // Parse EID to numeric TID (HEXADECIMAL for bit efficiency)
    static TID parseEidNumeric(const std::string& eid) {
        std::string numeric = eid.substr(0, 4) == "eid:" ? eid.substr(4) : eid;
        try {
            return static_cast<TID>(std::stoul(numeric, nullptr, 16));
        } catch (const std::exception&) {
            return INVALID_TID;
        }
    }
    
    // Generate EID string from TID
    static std::string generateEidString(const TID& tid) {
        if (tid == INVALID_TID) return "";
        std::ostringstream eidStream;
        eidStream << "eid:" << std::hex << tid;
        return eidStream.str();
    }

private:

    std::string _name;    // Human-readable name: "granite"
    std::string _eid;     // System EID: "eid:6" 
    TID _tid;            // Internal TID for storage
    bool _exists;        // Whether entity exists in store
};

} // namespace LabDb
