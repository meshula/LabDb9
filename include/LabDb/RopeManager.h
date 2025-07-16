// include/LabDb/RopeManager.h
#pragma once

#include <memory>
#include <string>
#include <vector>

// Forward declarations
class LabDb;
class Rope;

/**
 * RopeManager - Factory and lifecycle manager for rope operations
 * 
 * Manages rope creation, deletion, and listing operations using LabDb's
 * S-expression interface. Maintains weak reference to database for proper
 * lifecycle separation - rope manager does not own the database.
 * 
 * Key Design Principles:
 * - Weak pointer coupling to avoid circular dependencies
 * - S-expression native operations (no JSON)
 * - Factory pattern for rope creation
 * - Graceful degradation when database is destroyed
 */
class RopeManager {
private:
    std::weak_ptr<LabDb> db_weak_;
    std::string dbid_;
    
    /// Helper to get valid db pointer or throw
    std::shared_ptr<LabDb> get_db() const;
    
    /// Helper to parse S-expression success responses
    bool is_success_response(const std::string& response) const;
    
    /// Helper to parse rope list from S-expression response
    std::vector<std::string> parse_rope_list(const std::string& sexpr_response) const;

public:
    /**
     * Construct RopeManager with weak database reference
     * 
     * @param db Weak pointer to LabDb instance
     * @param dbid Database identifier for operations
     */
    explicit RopeManager(std::weak_ptr<LabDb> db, const std::string& dbid);
    
    /// Destructor - does not affect database lifecycle
    ~RopeManager() = default;
    
    // Non-copyable but movable
    RopeManager(const RopeManager&) = delete;
    RopeManager& operator=(const RopeManager&) = delete;
    RopeManager(RopeManager&&) = default;
    RopeManager& operator=(RopeManager&&) = default;
    
    // === Rope Factory Methods ===
    
    /**
     * Create new rope with optional description
     * 
     * @param rope_name Unique rope identifier
     * @param description Human-readable description (optional)
     * @return Unique pointer to Rope object, or nullptr on failure
     */
    std::unique_ptr<Rope> create_rope(const std::string& rope_name, 
                                     const std::string& description = "");
    
    /**
     * Get existing rope by name
     * 
     * @param rope_name Rope identifier
     * @return Unique pointer to Rope object, or nullptr if not found
     */
    std::unique_ptr<Rope> get_rope(const std::string& rope_name);
    
    // === Rope Management ===
    
    /**
     * Check if rope exists in database
     * 
     * @param rope_name Rope identifier to check
     * @return true if rope exists, false otherwise
     */
    bool rope_exists(const std::string& rope_name) const;
    
    /**
     * List all ropes in current database
     * 
     * @return Vector of rope names
     */
    std::vector<std::string> list_ropes() const;
    
    /**
     * Delete rope and all its adjacency records
     * 
     * @param rope_name Rope to delete
     * @return true on success, false on failure
     */
    bool delete_rope(const std::string& rope_name);
    
    /**
     * Delete all ropes in current database
     * 
     * @return true on success, false on failure
     */
    bool delete_all_ropes();
    
    // === Manager State ===
    
    /**
     * Check if manager has valid database connection
     * 
     * @return true if database connection is alive
     */
    bool is_valid() const;
    
    /**
     * Get current database identifier
     * 
     * @return Database ID string
     */
    const std::string& get_dbid() const { return dbid_; }
};