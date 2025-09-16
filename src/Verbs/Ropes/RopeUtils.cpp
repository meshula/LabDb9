#include "RopeUtils.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace LabDb {
namespace RopeUtils {

std::string generateSequenceId(size_t position) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(9) << (position * 1000);
    return oss.str();
}

size_t parseSequenceId(const std::string& sequence_id) {
    try {
        size_t seq_num = std::stoull(sequence_id);
        return seq_num / 1000;  // Convert back to position
    } catch (const std::exception&) {
        return 0;
    }
}

bool isValidRopeName(const std::string& name) {
    if (name.empty() || name.length() > 64) {
        return false;
    }
    
    // Check for valid characters: alphanumeric, hyphen, underscore
    for (char c : name) {
        if (!std::isalnum(c) && c != '-' && c != '_') {
            return false;
        }
    }
    
    return true;
}

std::string generateRopeKey(const std::string& rope_name, const std::string& sequence_id) {
    return "rope:" + rope_name + ":" + sequence_id;
}

RopeKeyComponents parseRopeKey(const std::string& key) {
    RopeKeyComponents components;
    components.valid = false;
    
    if (key.substr(0, 5) != "rope:") {
        return components;
    }
    
    size_t first_colon = key.find(':', 5);
    if (first_colon == std::string::npos) {
        return components;
    }
    
    components.rope_name = key.substr(5, first_colon - 5);
    components.sequence_id = key.substr(first_colon + 1);
    components.valid = true;
    
    return components;
}

std::string formatRopeMetadata(const std::string& rope_name, 
                              size_t entity_count, 
                              const std::string& description) {
    std::ostringstream json;
    json << "{"
         << "\"rope_name\": \"" << rope_name << "\""
         << ", \"entity_count\": " << entity_count
         << ", \"description\": \"" << description << "\""
         << "}";
    return json.str();
}

ChunkBounds calculateChunkBounds(size_t center_position, 
                               size_t chunk_size, 
                               size_t total_entities) {
    ChunkBounds bounds;
    
    // Calculate half chunk size for centering
    size_t half_chunk = chunk_size / 2;
    
    // Calculate start position (don't go below 0)
    if (center_position >= half_chunk) {
        bounds.start_position = center_position - half_chunk;
    } else {
        bounds.start_position = 0;
    }
    
    // Calculate end position (don't exceed total)
    bounds.end_position = std::min(bounds.start_position + chunk_size - 1, total_entities - 1);
    
    // Adjust start if we hit the end boundary
    if (bounds.end_position == total_entities - 1 && total_entities >= chunk_size) {
        bounds.start_position = total_entities - chunk_size;
    }
    
    bounds.center_position = center_position;
    bounds.can_extend_backward = (bounds.start_position > 0);
    bounds.can_extend_forward = (bounds.end_position < total_entities - 1);
    
    return bounds;
}

EntityPosition findEntityPosition(const std::string& rope_name,
                                const std::string& entity_id,
                                const std::string& dbid) {
    EntityPosition result;
    result.found = false;
    result.position = 0;
    
    // TODO: Implement entity position lookup via database scan
    // This will require iterating through rope keys to find matching entity
    
    return result;
}

std::string formatChunkResponse(const std::string& rope_name,
                              const std::string& center_entity,
                              const std::vector<std::string>& entity_ids,
                              const std::vector<std::string>& text_fragments,
                              size_t global_position) {
    std::ostringstream json;
    json << "{"
         << "\"status\": \"chunk_retrieved\""
         << ", \"rope_name\": \"" << rope_name << "\""
         << ", \"center_entity\": \"" << center_entity << "\""
         << ", \"fragment_count\": " << entity_ids.size()
         << ", \"global_position\": " << global_position
         << ", \"entities\": [";
    
    for (size_t i = 0; i < entity_ids.size(); ++i) {
        if (i > 0) json << ", ";
        json << "\"" << entity_ids[i] << "\"";
    }
    
    json << "]";
    
    if (!text_fragments.empty()) {
        json << ", \"text_fragments\": [";
        for (size_t i = 0; i < text_fragments.size(); ++i) {
            if (i > 0) json << ", ";
            json << "\"" << text_fragments[i] << "\"";  // TODO: Proper JSON escaping
        }
        json << "]";
    }
    
    json << "}";
    return json.str();
}

std::string RopeContext::toJsonString() const {
    std::ostringstream json;
    json << "{"
         << "\"rope_name\": \"" << rope_name << "\""
         << ", \"current_position\": " << current_position
         << ", \"total_entities\": " << total_entities
         << ", \"navigation_type\": \"" << navigation_type << "\""
         << ", \"boundary_aware\": " << (boundary_aware ? "true" : "false")
         << ", \"traversal_context\": \"" << traversal_context << "\""
         << "}";
    return json.str();
}

RopeContext createRopeContext(const std::string& rope_name,
                             size_t position,
                             const std::string& dbid) {
    RopeContext context;
    context.rope_name = rope_name;
    context.current_position = position;
    context.total_entities = 0;  // TODO: Query actual rope length
    context.navigation_type = "sequential";
    context.boundary_aware = true;
    context.traversal_context = "rope navigation";
    
    return context;
}

std::string createNavigationGuidance(const RopeContext& context,
                                    const std::string& operation) {
    std::ostringstream guidance;
    guidance << "Navigation guidance for operation '" << operation << "' "
             << "on rope '" << context.rope_name << "' "
             << "at position " << context.current_position;
    
    return guidance.str();
}

} // namespace RopeUtils
} // namespace LabDb