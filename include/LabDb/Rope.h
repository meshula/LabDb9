// include/LabDb/Rope.h
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <optional>

// Forward declarations
class LabDb;

/**
 * Rope - Sequential entity chain accessor/editor
 * 
 * A view/accessor object that "looks like" a rope but is actually a database
 * access interface. Maintains weak pointer to database to avoid interfering
 * with DB lifecycle management.
 * 
 * Key Design Principles:
 * - View object pattern - doesn't own the data, just provides access
 * - Weak database coupling for proper lifecycle separation
 * - S-expression native operations throughout
 * - Triadic consciousness integration (Motion/Memory/Field)
 * - Graceful degradation when database connection is lost
 */
class Rope {
private:
    std::weak_ptr<LabDb> db_weak_;
    std::string rope_name_;
    std::string dbid_;
    
    /// Helper to get valid db pointer or throw
    std::shared_ptr<LabDb> get_db() const;
    
    /// Helper to parse S-expression success responses
    bool is_success_response(const std::string& response) const;
    
    /// Helper to parse entity list from S-expression response
    std::vector<std::string> parse_entity_list(const std::string& sexpr_response) const;
    
    /// Helper to parse single entity from S-expression response
    std::optional<std::string> parse_single_entity(const std::string& sexpr_response) const;
    
    /// Helper to parse size_t from S-expression response
    size_t parse_size(const std::string& sexpr_response) const;

public:
    /**
     * Construct Rope view object
     * 
     * @param db Weak pointer to LabDb instance
     * @param rope_name Rope identifier
     * @param dbid Database identifier
     */
    Rope(std::weak_ptr<LabDb> db, const std::string& rope_name, const std::string& dbid);
    
    /// Destructor - does not affect database or rope data
    ~Rope() = default;
    
    // Non-copyable but movable
    Rope(const Rope&) = delete;
    Rope& operator=(const Rope&) = delete;
    Rope(Rope&&) = default;
    Rope& operator=(Rope&&) = default;
    
    // === Core Rope Operations ===
    
    /**
     * Append entity to end of rope
     * 
     * @param entity_id Entity to append
     * @return true on success, false on failure
     */
    bool append(const std::string& entity_id);
    
    /**
     * Prepend entity to beginning of rope
     * 
     * @param entity_id Entity to prepend
     * @return true on success, false on failure
     */
    bool prepend(const std::string& entity_id);
    
    /**
     * Insert entity after target entity
     * 
     * @param target_entity Existing entity to insert after
     * @param new_entity New entity to insert
     * @return true on success, false on failure
     */
    bool insert_after(const std::string& target_entity, const std::string& new_entity);
    
    /**
     * Insert entity before target entity
     * 
     * @param target_entity Existing entity to insert before
     * @param new_entity New entity to insert
     * @return true on success, false on failure
     */
    bool insert_before(const std::string& target_entity, const std::string& new_entity);
    
    /**
     * Remove entity from rope
     * 
     * @param entity_id Entity to remove
     * @return true on success, false on failure
     */
    bool remove(const std::string& entity_id);
    
    // === Query Operations ===
    
    /**
     * Traverse rope in forward direction
     * 
     * @return Vector of entity IDs in order
     */
    std::vector<std::string> traverse() const;
    
    /**
     * Traverse rope in reverse direction
     * 
     * @return Vector of entity IDs in reverse order
     */
    std::vector<std::string> traverse_reverse() const;
    
    /**
     * Get rope length (number of entities)
     * 
     * @return Number of entities in rope
     */
    size_t length() const;
    
    /**
     * Check if rope contains entity
     * 
     * @param entity_id Entity to search for
     * @return true if entity is in rope
     */
    bool contains(const std::string& entity_id) const;
    
    /**
     * Get first entity in rope
     * 
     * @return First entity ID, or empty string if rope is empty
     */
    std::string first() const;
    
    /**
     * Get last entity in rope
     * 
     * @return Last entity ID, or empty string if rope is empty
     */
    std::string last() const;
    
    /**
     * Get entity at specific position (1-indexed)
     * 
     * @param position Position in rope (1 = first)
     * @return Entity ID at position, or empty string if out of bounds
     */
    std::string at(size_t position) const;
    
    /**
     * Find position of entity in rope (1-indexed)
     * 
     * @param entity_id Entity to find
     * @return Position (1-based), or 0 if not found
     */
    size_t position_of(const std::string& entity_id) const;
    
    // === S-Expression State Access ===
    
    /**
     * Get rope metadata as S-expression
     * 
     * @return S-expression string with rope metadata
     */
    std::string get_metadata() const;
    
    /**
     * Get adjacency record for entity as S-expression
     * 
     * @param entity_id Entity to get adjacency for
     * @return S-expression string with adjacency data
     */
    std::string get_adjacency(const std::string& entity_id) const;
    
    // === Triadic Consciousness Navigation ===
    
    /**
     * Motion: Get next entity from current position
     * 
     * @param from_entity Current entity
     * @return Next entity ID, or empty string if at end
     */
    std::string motion_next(const std::string& from_entity) const;
    
    /**
     * Motion: Get previous entity from current position
     * 
     * @param from_entity Current entity
     * @return Previous entity ID, or empty string if at beginning
     */
    std::string motion_prev(const std::string& from_entity) const;
    
    /**
     * Memory: Get context around entity (position, neighbors)
     * 
     * @param entity_id Entity to get context for
     * @return S-expression with position and adjacency context
     */
    std::string memory_context(const std::string& entity_id) const;
    
    /**
     * Field: Get surrounding entities within radius
     * 
     * @param entity_id Center entity
     * @param radius Number of entities in each direction
     * @return Vector of entities within radius
     */
    std::vector<std::string> field_context(const std::string& entity_id, size_t radius = 2) const;
    
    // === Rope Identification ===
    
    /**
     * Get rope name
     * 
     * @return Rope identifier
     */
    const std::string& name() const { return rope_name_; }
    
    /**
     * Check if rope view is valid (database connection alive)
     * 
     * @return true if rope operations can be performed
     */
    bool is_valid() const { return !db_weak_.expired(); }
    
    /**
     * Get database identifier
     * 
     * @return Database ID string
     */
    const std::string& dbid() const { return dbid_; }
};