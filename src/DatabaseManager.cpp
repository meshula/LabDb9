#include "LabDb/DatabaseManager.h"
#include "LabDb/DatabaseVerbs.h"
#include "LabDb/EntityId.h"
#include "LabDb/LabText.hpp"
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iostream>

namespace LabDb {

//-----------------------------------------------------------------------------
// DatabaseManager Implementation
//-----------------------------------------------------------------------------

std::string DatabaseManager::openDatabase(const std::string& path) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    try {
        // Check if file exists
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("Database file does not exist: " + path);
        }
        
        // Create NonoStore instance
        auto store = std::make_shared<NonoStore>(path);
        
        // Generate unique DBID
        std::string dbid = generateDbid();
        
        // Store in registry
        _databases[dbid] = store;
        
        return dbid;
        
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to open database: " + std::string(e.what()));
    }
}

bool DatabaseManager::closeDatabase(const std::string& dbid) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _databases.find(dbid);
    if (it == _databases.end()) {
        return false;
    }
    
    _databases.erase(it);
    return true;
}

std::shared_ptr<NonoStore> DatabaseManager::getDatabase(const std::string& dbid) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _databases.find(dbid);
    if (it == _databases.end()) {
        return nullptr;
    }
    
    return it->second;
}

bool DatabaseManager::isValidDbid(const std::string& dbid) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _databases.find(dbid) != _databases.end();
}

std::vector<std::string> DatabaseManager::getActiveDbids() const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::vector<std::string> dbids;
    dbids.reserve(_databases.size());
    
    for (const auto& pair : _databases) {
        dbids.push_back(pair.first);
    }
    
    return dbids;
}

std::string DatabaseManager::generateDbid() {
    return "db" + std::to_string(_next_id++);
}

std::string DatabaseManager::createDatabase(const std::string& path) {
    std::lock_guard<std::mutex> lock(_mutex);

    // if path is relative,
    if (!std::filesystem::path(path).is_absolute()) {
        // get the current working directory for relative paths
        std::string cwd = std::filesystem::current_path().string();
        // if cwd is empty, throw and say we need absolute path
        if (cwd.empty()) {
            throw std::runtime_error("Current working directory is not set; please provide an absolute path.");
        }
    }

    try {
        // Check if file already exists
        if (std::filesystem::exists(path)) {
            throw std::runtime_error("Database file already exists: " + path);
        }

        // Ensure parent directory exists
        std::filesystem::path db_path(path);
        std::filesystem::path parent_dir = db_path.parent_path();
        if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
            std::filesystem::create_directories(parent_dir);
        }

        // Create NonoStore instance (which creates/initializes the database)
        auto store = std::make_shared<NonoStore>(path);

        // Generate unique DBID
        std::string dbid = generateDbid();

        // Store in registry
        _databases[dbid] = store;

        return dbid;

    } catch (const std::exception& e) {
        std::string msg = e.what();
        std::string cwd = std::filesystem::current_path().string();
        msg += "\nCurrent working directory: " + cwd;
        msg += "\nPlease ensure the path is correct and writable.";
        throw std::runtime_error("Failed to create database: " + std::string(e.what()));
    }
}

} // namespace LabDb
