// src/RopeManager.cpp
#include "LabDb/RopeManager.h"
#include "LabDb/Rope.h"
#include "LabDb/LabDb.h"
#include <stdexcept>
#include <sstream>

// Constructor
RopeManager::RopeManager(std::weak_ptr<LabDb> db, const std::string& dbid)
    : db_weak_(db), dbid_(dbid) 
{
    // Validate that database connection is initially valid
    if (db_weak_.expired()) {
        throw std::invalid_argument("Cannot create RopeManager with expired database reference");
    }
}

// Private helper methods
std::shared_ptr<LabDb> RopeManager::get_db() const {
    auto db = db_weak_.lock();
    if (!db) {
        throw std::runtime_error("Database connection expired for RopeManager");
    }
    return db;
}

bool RopeManager::is_success_response(const std::string& response) const {
    // TODO: Implement proper S-expression parsing
    // For now, simple string matching
    return response.find(":status \"success\"") != std::string::npos ||
           response.find(":status success") != std::string::npos ||
           response.find("success") != std::string::npos;
}

std::vector<std::string> RopeManager::parse_rope_list(const std::string& sexpr_response) const {
    // TODO: Implement proper S-expression list parsing
    // For now, return empty vector
    std::vector<std::string> ropes;
    
    // Placeholder implementation - needs real S-expression parser
    // Expected format: (rope-list :ropes (§rope1§ §rope2§ §rope3§))
    
    return ropes;
}

// Public interface methods
std::unique_ptr<Rope> RopeManager::create_rope(const std::string& rope_name, 
                                              const std::string& description) {
    auto db = get_db();
    
    // Create rope via S-expression command
    std::ostringstream cmd;
    cmd << "(rope-create :name §" << rope_name << "§ :description §" 
        << description << "§ :dbid §" << dbid_ << "§)";
    
    auto result = db->execute_sexpr(cmd.str());
    if (!is_success_response(result)) {
        return nullptr;
    }
    
    return std::make_unique<Rope>(db_weak_, rope_name, dbid_);
}

std::unique_ptr<Rope> RopeManager::get_rope(const std::string& rope_name) {
    if (!rope_exists(rope_name)) {
        return nullptr;
    }
    return std::make_unique<Rope>(db_weak_, rope_name, dbid_);
}

bool RopeManager::rope_exists(const std::string& rope_name) const {
    auto db = get_db();
    
    std::ostringstream cmd;
    cmd << "(rope-exists :name §" << rope_name << "§ :dbid §" << dbid_ << "§)";
    
    auto result = db->execute_sexpr(cmd.str());
    return is_success_response(result);
}

std::vector<std::string> RopeManager::list_ropes() const {
    auto db = get_db();
    
    std::ostringstream cmd;
    cmd << "(rope-list :dbid §" << dbid_ << "§)";
    
    auto result = db->execute_sexpr(cmd.str());
    return parse_rope_list(result);
}

bool RopeManager::delete_rope(const std::string& rope_name) {
    auto db = get_db();
    
    std::ostringstream cmd;
    cmd << "(rope-delete :name §" << rope_name << "§ :dbid §" << dbid_ << "§)";
    
    auto result = db->execute_sexpr(cmd.str());
    return is_success_response(result);
}

bool RopeManager::delete_all_ropes() {
    auto db = get_db();
    
    std::ostringstream cmd;
    cmd << "(rope-delete-all :dbid §" << dbid_ << "§)";
    
    auto result = db->execute_sexpr(cmd.str());
    return is_success_response(result);
}

bool RopeManager::is_valid() const {
    return !db_weak_.expired();
}