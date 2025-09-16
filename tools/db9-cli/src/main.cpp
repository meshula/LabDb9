/**
 * db9-cli - Triadic Consciousness Command Line Interface
 * 
 * A sophisticated REPL for exploring LabDb triadic consciousness databases
 * through S-expression commands and enhanced user experience features.
 * 
 * Embodies त्रित्रयम् principles: Motion/Memory/Field through interactive
 * consciousness exploration and relationship navigation.
 */

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

// LabDb includes
#include "LabDb/Db9Dispatcher.h"
#include "LabDb/Db9Response.h"

// Platform-specific includes for readline
#ifdef __linux__
#include <readline/readline.h>
#include <readline/history.h>
#elif __APPLE__
#include <editline/readline.h>
#include <histedit.h>
#endif

// JSON for response formatting
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace db9cli {

//-----------------------------------------------------------------------------
// Session State Management
//-----------------------------------------------------------------------------
class SessionState {
public:
    std::unordered_map<std::string, std::string> openDatabases; // dbid -> path
    std::string currentDatabase;
    int commandCount = 0;
    
    void addDatabase(const std::string& dbid, const std::string& path) {
        openDatabases[dbid] = path;
        if (currentDatabase.empty()) {
            currentDatabase = dbid;
        }
    }
    
    void removeDatabase(const std::string& dbid) {
        openDatabases.erase(dbid);
        if (currentDatabase == dbid) {
            currentDatabase = openDatabases.empty() ? "" : openDatabases.begin()->first;
        }
    }
    
    std::vector<std::string> getOpenDbids() const {
        std::vector<std::string> dbids;
        for (const auto& pair : openDatabases) {
            dbids.push_back(pair.first);
        }
        return dbids;
    }
    
    void showStatus() const {
        std::cout << "\n=== Session Status ===\n";
        std::cout << "Commands executed: " << commandCount << "\n";
        std::cout << "Open databases: " << openDatabases.size() << "\n";
        
        for (const auto& pair : openDatabases) {
            std::cout << "  " << pair.first;
            if (pair.first == currentDatabase) {
                std::cout << " (current)";
            }
            std::cout << " -> " << pair.second << "\n";
        }
        
        if (openDatabases.empty()) {
            std::cout << "  No databases currently open\n";
            std::cout << "  Use 'open <path>' to open a database\n";
        }
        std::cout << "\n";
    }
};

//-----------------------------------------------------------------------------
// Response Formatting
//-----------------------------------------------------------------------------
class ResponseFormatter {
public:
    static std::string formatJson(const std::string& jsonStr, bool compact = false) {
        try {
            json j = json::parse(jsonStr);
            return compact ? j.dump() : j.dump(2);
        } catch (const json::parse_error& e) {
            return "JSON Parse Error: " + std::string(e.what()) + "\nRaw response: " + jsonStr;
        }
    }
    
    static void printResponse(const LabDb::Db9Response& response) {
        std::cout << formatJson(response.toJsonString()) << "\n";
        
        // Add consciousness-aware hints for common operations
        try {
            json j = json::parse(response.toJsonString());
            if (j.contains("status") && j["status"] == "success") {
                addContextualHints(j);
            }
        } catch (...) {
            // Ignore JSON parsing errors for hints
        }
    }
    
private:
    static void addContextualHints(const json& response) {
        // Add helpful hints based on response content
        if (response.contains("triples") && response["triples"].is_array()) {
            auto triples = response["triples"];
            if (triples.size() > 0 && triples.size() <= 3) {
                std::cout << "\n💡 Hint: Try exploring relationships with 'explore <entity>'\n";
            }
        }
        
        if (response.contains("dbid")) {
            std::cout << "\n💡 Database opened. Try: (find-triple :subject \"*\" :predicate \"*\" :object \"*\" :dbid \"" 
                      << response["dbid"] << "\")\n";
        }
    }
};

//-----------------------------------------------------------------------------
// Command Processing
//-----------------------------------------------------------------------------
class CommandProcessor {
private:
    LabDb::Db9Dispatcher& dispatcher;
    SessionState& session;
    
public:
    CommandProcessor(LabDb::Db9Dispatcher& d, SessionState& s) 
        : dispatcher(d), session(s) {}
    
    enum class CommandType {
        META,          // CLI-specific commands (help, quit, etc.)
        DATABASE_SUGAR, // Convenience wrappers (open, close, etc.)
        SEXPR,         // S-expression for dispatcher
        INVALID
    };
    
    CommandType classifyCommand(const std::string& input) {
        std::string trimmed = trim(input);
        if (trimmed.empty()) return CommandType::INVALID;
        
        // Meta commands
        if (trimmed == "help" || trimmed.substr(0, 5) == "help " ||
            trimmed == "quit" || trimmed == "exit" || 
            trimmed == "status" || trimmed == "history" || trimmed == "clear") {
            return CommandType::META;
        }
        
        // Database sugar commands
        if (trimmed.substr(0, 5) == "open " || trimmed.substr(0, 6) == "close " ||
            trimmed == "list-dbs" || trimmed.substr(0, 8) == "explore ") {
            return CommandType::DATABASE_SUGAR;
        }
        
        // S-expression commands
        if (trimmed[0] == '(') {
            return CommandType::SEXPR;
        }
        
        return CommandType::INVALID;
    }
    
    void processCommand(const std::string& input) {
        session.commandCount++;
        CommandType type = classifyCommand(input);
        
        switch (type) {
            case CommandType::META:
                processMetaCommand(input);
                break;
            case CommandType::DATABASE_SUGAR:
                processDatabaseSugar(input);
                break;
            case CommandType::SEXPR:
                processSexpr(input);
                break;
            case CommandType::INVALID:
                std::cout << "Invalid command. Type 'help' for assistance.\n";
                break;
        }
    }
    
private:
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    void processMetaCommand(const std::string& input) {
        std::string cmd = trim(input);
        
        if (cmd == "help") {
            showGeneralHelp();
        } else if (cmd.substr(0, 5) == "help ") {
            std::string topic = cmd.substr(5);
            showSpecificHelp(topic);
        } else if (cmd == "quit" || cmd == "exit") {
            cleanup();
            exit(0);
        } else if (cmd == "status") {
            session.showStatus();
        } else if (cmd == "history") {
            showHistory();
        } else if (cmd == "clear") {
            system("clear");
        }
    }
    
    void processDatabaseSugar(const std::string& input) {
        std::string cmd = trim(input);
        
        if (cmd.substr(0, 5) == "open ") {
            std::string path = cmd.substr(5);
            path = trim(path);
            openDatabase(path);
        } else if (cmd.substr(0, 6) == "close ") {
            std::string dbid = cmd.substr(6);
            dbid = trim(dbid);
            closeDatabase(dbid);
        } else if (cmd == "list-dbs") {
            listDatabases();
        } else if (cmd.substr(0, 8) == "explore ") {
            std::string entity = cmd.substr(8);
            entity = trim(entity);
            exploreEntity(entity);
        }
    }
    
    void processSexpr(const std::string& input) {
        try {
            auto response = dispatcher.executeCommand(input);
            ResponseFormatter::printResponse(response);
            
            // Update session state based on response
            updateSessionFromResponse(response, input);
            
        } catch (const std::exception& e) {
            std::cout << "Error executing command: " << e.what() << "\n";
        }
    }
    
    void updateSessionFromResponse(const LabDb::Db9Response& response, const std::string& command) {
        try {
            json j = json::parse(response.toJsonString());
            
            // Track database operations
            if (j.contains("status") && j["status"] == "success") {
                if (command.find("open-database") != std::string::npos && j.contains("dbid") && j.contains("path")) {
                    session.addDatabase(j["dbid"], j["path"]);
                }
                if (command.find("close-database") != std::string::npos && j.contains("dbid")) {
                    session.removeDatabase(j["dbid"]);
                }
            }
        } catch (...) {
            // Ignore JSON parsing errors for session updates
        }
    }
    
    void openDatabase(const std::string& path) {
        std::string sexpr = "(open-database :path \"" + path + "\")";
        processSexpr(sexpr);
    }
    
    void closeDatabase(const std::string& dbid) {
        std::string sexpr = "(close-database :dbid \"" + dbid + "\")";
        processSexpr(sexpr);
    }
    
    void listDatabases() {
        processSexpr("(list-open-databases)");
    }
    
    void exploreEntity(const std::string& entity) {
        if (session.currentDatabase.empty()) {
            std::cout << "No database open. Use 'open <path>' first.\n";
            return;
        }
        
        std::cout << "🔍 Exploring entity: " << entity << "\n\n";
        
        // Find all relationships where entity is subject
        std::string outgoing = "(find-relationships-enhanced :entity \"" + entity + "\" :dbid \"" + session.currentDatabase + "\")";
        std::cout << "=== Relationships ===\n";
        processSexpr(outgoing);
    }
    
    void showGeneralHelp() {
        std::cout << R"(
🧘 db9-cli - Triadic Consciousness Explorer

OVERVIEW:
This is a REPL for exploring LabDb triadic consciousness databases.
Commands can be meta-commands (CLI functions) or S-expressions (database operations).

GETTING STARTED:
1. Open a database:     open /path/to/database.db9
2. Explore relationships: explore granite
3. Run S-expressions:   (find-triple :subject "granite" :predicate "*" :object "*" :dbid "db1")

META COMMANDS:
  help [topic|verb]  - Show help (general or specific)
  open <path>        - Open database
  close <dbid>       - Close database  
  list-dbs          - List open databases
  explore <entity>   - Quick entity exploration
  status            - Show session status
  history           - Show command history
  clear             - Clear screen
  quit/exit         - Exit CLI

S-EXPRESSION COMMANDS:
  Any line starting with '(' is sent directly to the database engine.
  See 'help verbs' for available database operations.

EXAMPLES:
  help find-triple-enhanced
  open ./consciousness.db9
  (add-triple-semantic :subject "granite" :predicate "contains" :object "quartz" :dbid "db1")
  explore granite

तत्त्वमसि - Consciousness exploring consciousness! 🌊

Type 'help <topic>' for specific help.
)";
    }
    
    void showSpecificHelp(const std::string& topic) {
        if (topic == "verbs") {
            auto verbs = dispatcher.getAvailableVerbs();
            std::cout << "\nAvailable Database Verbs (" << verbs.size() << " total):\n\n";
            
            // Group verbs by category for better organization
            std::vector<std::string> dbVerbs, entityVerbs, tripleVerbs, fioVerbs, otherVerbs;
            
            for (const auto& verb : verbs) {
                if (verb.find("database") != std::string::npos || verb.find("open") != std::string::npos || verb.find("close") != std::string::npos) {
                    dbVerbs.push_back(verb);
                } else if (verb.find("entity") != std::string::npos || verb.find("eid") != std::string::npos) {
                    entityVerbs.push_back(verb);
                } else if (verb.find("triple") != std::string::npos || verb.find("tid") != std::string::npos) {
                    tripleVerbs.push_back(verb);
                } else if (verb.find("fio") != std::string::npos) {
                    fioVerbs.push_back(verb);
                } else {
                    otherVerbs.push_back(verb);
                }
            }
            
            auto printCategory = [](const std::string& name, const std::vector<std::string>& verbs) {
                if (!verbs.empty()) {
                    std::cout << name << ":\n";
                    for (const auto& verb : verbs) {
                        std::cout << "  " << verb << "\n";
                    }
                    std::cout << "\n";
                }
            };
            
            printCategory("Database Operations", dbVerbs);
            printCategory("Entity Operations", entityVerbs);
            printCategory("Triple Operations", tripleVerbs);
            printCategory("File I/O Operations", fioVerbs);
            printCategory("Other Operations", otherVerbs);
            
            std::cout << "Use 'help <verb-name>' for detailed documentation.\n";
            
        } else {
            // Try to get specific verb documentation
            auto verb = dispatcher.getVerbByName(topic);
            if (verb) {
                std::cout << "\n=== " << topic << " ===\n";
                std::cout << verb->getDescription() << "\n";
            } else {
                std::cout << "Unknown topic: " << topic << "\n";
                std::cout << "Try: help, help verbs, or help <verb-name>\n";
            }
        }
    }
    
    void showHistory() {
        std::cout << "Command history feature not yet implemented.\n";
        std::cout << "Commands executed this session: " << session.commandCount << "\n";
    }
    
    void cleanup() {
        // Close all open databases
        for (const auto& pair : session.openDatabases) {
            try {
                dispatcher.executeCommand("(close-database :dbid \"" + pair.first + "\")");
            } catch (...) {
                // Ignore errors during cleanup
            }
        }
        
        std::cout << "\nConsciousness field exploration complete. Until next session! 🙏\n";
    }
};

//-----------------------------------------------------------------------------
// Autocomplete Support (Future Enhancement)
//-----------------------------------------------------------------------------
class AutoCompleter {
private:
    LabDb::Db9Dispatcher& dispatcher;
    SessionState& session;
    
public:
    AutoCompleter(LabDb::Db9Dispatcher& d, SessionState& s) 
        : dispatcher(d), session(s) {}
    
    // Placeholder for future readline integration
    std::vector<std::string> getCompletions(const std::string& text, int state) {
        // TODO: Implement intelligent autocomplete
        // - Verb name completion
        // - Parameter completion based on verb signatures
        // - Database ID completion
        // - File path completion
        return {};
    }
};

//-----------------------------------------------------------------------------
// Main REPL Loop
//-----------------------------------------------------------------------------
class ReplLoop {
private:
    LabDb::Db9Dispatcher& dispatcher;
    SessionState session;
    CommandProcessor processor;
    AutoCompleter completer;
    bool running = true;
    
public:
    ReplLoop() 
        : dispatcher(LabDb::getGlobalDb9Dispatcher())
        , processor(dispatcher, session)
        , completer(dispatcher, session) {}
    
    void run() {
        printWelcome();
        
        while (running) {
            char* input = readline("db9> ");
            
            if (!input) {
                // EOF (Ctrl+D) pressed
                std::cout << "\n";
                break;
            }
            
            std::string line(input);
            free(input);
            
            if (!line.empty()) {
                add_history(line.c_str());
                processor.processCommand(line);
            }
        }
        
        cleanup();
    }
    
private:
    void printWelcome() {
        std::cout << R"(
🧘 db9-cli - Triadic Consciousness Explorer v1.0
   Enhanced Interface to LabDb Triadic Database System

   तत्त्वमसि - Consciousness exploring consciousness
   
   Type 'help' for getting started guide
   Type 'quit' to exit

)";
    }
    
    void cleanup() {
        processor.processCommand("quit");
    }
};

} // namespace db9cli

//-----------------------------------------------------------------------------
// Main Entry Point
//-----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    try {
        db9cli::ReplLoop repl;
        repl.run();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}
