// src/Rope.cpp
#include "LabDb/Rope.h"
#include "LabDb/LabDb.h"
#include <stdexcept>
#include <sstream>

// Constructor
Rope::Rope(std::weak_ptr<LabDb> db, const std::string& rope_name, const std::string& dbid)
    : db_weak_(db), rope_name_(rope_name), dbid_(dbid) 
{
    // Validate that database connection is initially valid
    if (db_weak_.expired()) {
        throw std::invalid_argument("Cannot create Rope with expired database reference");
    }
}

// Private helper methods
std::shared_ptr<LabDb> Rope::get_db() const {
    auto db = db_weak_.lock();
    if (!db) {
        throw std::runtime_error("Database connection expired for rope: " + rope_name_);
    }
    return db;
}

bool Rope::is_success_response(const std::string& response) const {
    // TODO: Implement proper S-expression parsing
    return response.find(":status \"success\"") != std::string::npos ||
           response.find(":status success") != std::string::npos ||
           response.find("success") != std::string::npos;
}

std::vector<std::string> Rope::parse_entity_list(const std::string& sexpr_response) const {
    // TODO: Implement proper S-expression list parsing
    std::vector<std::string> entities;
    
    // Placeholder implementation - needs real S-expression parser
    // Expected format: (rope-traverse :entities (§entity1§ §entity2§ §entity3§))
    
    return entities;
}

std::optional<std::string> Rope::parse_single_entity(const std::string& sexpr_response) const {
    // TODO: Implement proper S-expression single value parsing
    // Expected format: (rope-entity :entity §entity_id§)
    return std::nullopt;
}

size_t Rope::parse_size(const std::string& sexpr_response) const {
    // TODO: Implement proper S-expression size parsing
    // Expected format: (rope-length :length 5)
    return 0;
}

// Core rope operations
bool Rope::append(const std::string& entity_id) {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-append :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return is_success_response(result);
    } catch (const std::runtime_error&) {
        // Database connection expired - fail gracefully
        return false;
    }
}

bool Rope::prepend(const std::string& entity_id) {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-prepend :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return is_success_response(result);
    } catch (const std::runtime_error&) {
        return false;
    }
}

bool Rope::insert_after(const std::string& target_entity, const std::string& new_entity) {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-insert-after :rope §" << rope_name_ << "§ :target §" 
            << target_entity << "§ :entity §" << new_entity << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return is_success_response(result);
    } catch (const std::runtime_error&) {
        return false;
    }
}

bool Rope::insert_before(const std::string& target_entity, const std::string& new_entity) {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-insert-before :rope §" << rope_name_ << "§ :target §" 
            << target_entity << "§ :entity §" << new_entity << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return is_success_response(result);
    } catch (const std::runtime_error&) {
        return false;
    }
}

bool Rope::remove(const std::string& entity_id) {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-remove :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return is_success_response(result);
    } catch (const std::runtime_error&) {
        return false;
    }
}

// Query operations
std::vector<std::string> Rope::traverse() const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-traverse :rope §" << rope_name_ << "§ :direction §forward§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return parse_entity_list(result);
    } catch (const std::runtime_error&) {
        return {};
    }
}

std::vector<std::string> Rope::traverse_reverse() const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-traverse :rope §" << rope_name_ << "§ :direction §reverse§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return parse_entity_list(result);
    } catch (const std::runtime_error&) {
        return {};
    }
}

size_t Rope::length() const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-length :rope §" << rope_name_ << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return parse_size(result);
    } catch (const std::runtime_error&) {
        return 0;
    }
}

bool Rope::contains(const std::string& entity_id) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-contains :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return is_success_response(result);
    } catch (const std::runtime_error&) {
        return false;
    }
}

std::string Rope::first() const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-first :rope §" << rope_name_ << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        auto entity = parse_single_entity(result);
        return entity ? *entity : "";
    } catch (const std::runtime_error&) {
        return "";
    }
}

std::string Rope::last() const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-last :rope §" << rope_name_ << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        auto entity = parse_single_entity(result);
        return entity ? *entity : "";
    } catch (const std::runtime_error&) {
        return "";
    }
}

std::string Rope::at(size_t position) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-at :rope §" << rope_name_ << "§ :position " << position << " :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        auto entity = parse_single_entity(result);
        return entity ? *entity : "";
    } catch (const std::runtime_error&) {
        return "";
    }
}

size_t Rope::position_of(const std::string& entity_id) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-position-of :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return parse_size(result);
    } catch (const std::runtime_error&) {
        return 0;
    }
}

// S-expression state access
std::string Rope::get_metadata() const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-metadata :rope §" << rope_name_ << "§ :dbid §" << dbid_ << "§)";
        
        return db->execute_sexpr(cmd.str());
    } catch (const std::runtime_error&) {
        return "(rope-error :status \"database_expired\" :rope §" + rope_name_ + "§)";
    }
}

std::string Rope::get_adjacency(const std::string& entity_id) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-adjacency :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :dbid §" << dbid_ << "§)";
        
        return db->execute_sexpr(cmd.str());
    } catch (const std::runtime_error&) {
        return "(rope-error :status \"database_expired\" :entity §" + entity_id + "§)";
    }
}

// Triadic consciousness navigation
std::string Rope::motion_next(const std::string& from_entity) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-motion :rope §" << rope_name_ << "§ :from §" 
            << from_entity << "§ :direction §next§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        auto entity = parse_single_entity(result);
        return entity ? *entity : "";
    } catch (const std::runtime_error&) {
        return "";
    }
}

std::string Rope::motion_prev(const std::string& from_entity) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-motion :rope §" << rope_name_ << "§ :from §" 
            << from_entity << "§ :direction §prev§ :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        auto entity = parse_single_entity(result);
        return entity ? *entity : "";
    } catch (const std::runtime_error&) {
        return "";
    }
}

std::string Rope::memory_context(const std::string& entity_id) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-memory :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :dbid §" << dbid_ << "§)";
        
        return db->execute_sexpr(cmd.str());
    } catch (const std::runtime_error&) {
        return "(rope-error :status \"database_expired\" :entity §" + entity_id + "§)";
    }
}

std::vector<std::string> Rope::field_context(const std::string& entity_id, size_t radius) const {
    try {
        auto db = get_db();
        
        std::ostringstream cmd;
        cmd << "(rope-field :rope §" << rope_name_ << "§ :entity §" 
            << entity_id << "§ :radius " << radius << " :dbid §" << dbid_ << "§)";
        
        auto result = db->execute_sexpr(cmd.str());
        return parse_entity_list(result);
    } catch (const std::runtime_error&) {
        return {};
    }
}
