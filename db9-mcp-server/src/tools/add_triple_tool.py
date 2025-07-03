"""
Add Triple Tool with Dual-Mode EID Resolution for DB9 MCP Server

Implements the conversational EID resolution pattern:
- Natural language: add_triple("rose_quartz", "has_color", "pink") 
- Enterprise explicit: add_triple("rose_quartz", "has_color", "eid:pink_003")

Supports disambiguation dialogs and auto-creation of missing entities.
"""

import logging
import re
from typing import Dict, List, Optional, Tuple, Any
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
import sys

# Add LabDb Python bindings to path
labdb_python_dir = Path(__file__).parent.parent.parent.parent / "python"
if labdb_python_dir.exists() and str(labdb_python_dir) not in sys.path:
    sys.path.insert(0, str(labdb_python_dir))

try:
    import labdb
    LABDB_AVAILABLE = True
except ImportError as e:
    logging.warning(f"LabDb Python bindings not available: {e}")
    LABDB_AVAILABLE = False

logger = logging.getLogger(__name__)


class ResolutionType(Enum):
    """Type of EID resolution performed"""
    EXPLICIT = "explicit"          # User provided "eid:..." - use directly  
    UNIQUE_MATCH = "unique_match"  # Single EID found for natural term
    AMBIGUOUS = "ambiguous"        # Multiple EIDs found - needs disambiguation
    NOT_FOUND = "not_found"        # No EIDs found - suggest creation
    AUTO_CREATED = "auto_created"  # New EID was created automatically


@dataclass
class EIDCandidate:
    """A candidate EID with metadata for disambiguation"""
    eid: str
    description: str
    entity_type: str
    confidence: float
    context: Dict[str, Any]


@dataclass 
class ResolutionResult:
    """Result of EID resolution attempt"""
    term: str                              # Original term provided
    resolution_type: ResolutionType       # Type of resolution
    resolved_eid: Optional[str] = None     # Final EID if resolved
    candidates: List[EIDCandidate] = None  # Available candidates if ambiguous
    confidence: float = 0.0                # Resolution confidence
    suggestion: Optional[str] = None       # Suggestion for creation if not found


class TriadicEIDResolver:
    """
    Dual-mode EID resolution for triadic consciousness database.
    
    Handles both explicit EIDs (eid:prefix) and natural language terms
    with intelligent auto-creation and disambiguation support.
    """
    
    def __init__(self, database_manager, auto_create: bool = True):
        """Initialize resolver with database manager"""
        self.db_manager = database_manager
        self.auto_create = auto_create
        self.logger = logging.getLogger(__name__)
        # Session cache for term -> EID mappings
        self.session_cache: Dict[str, str] = {}
    
    def resolve_triple_terms(self, subject: str, predicate: str, obj: str) -> Tuple[ResolutionResult, ResolutionResult, ResolutionResult]:
        """
        Resolve all three terms of a triple for add_triple operation.
        
        Returns tuple of (subject_result, predicate_result, object_result)
        """
        subject_result = self.resolve_term(subject, entity_role="subject")
        predicate_result = self.resolve_term(predicate, entity_role="predicate")  
        object_result = self.resolve_term(obj, entity_role="object")
        
        return subject_result, predicate_result, object_result
    
    def resolve_term(self, term: str, entity_role: str = "entity") -> ResolutionResult:
        """
        Resolve a single term to EID.
        
        Args:
            term: Natural language term or explicit EID
            entity_role: Role of entity in triple (for better EID generation)
            
        Returns:
            ResolutionResult with resolution outcome
        """
        # Check if explicit EID provided
        if term.startswith("eid:"):
            return self._resolve_explicit_eid(term)
        
        # Check session cache
        if term in self.session_cache:
            return ResolutionResult(
                term=term,
                resolution_type=ResolutionType.UNIQUE_MATCH,
                resolved_eid=self.session_cache[term],
                confidence=1.0
            )
        
        # Natural language resolution
        return self._resolve_natural_term(term, entity_role)
    
    def _resolve_explicit_eid(self, eid_term: str) -> ResolutionResult:
        """Handle explicit EID (eid:prefix) resolution"""
        # Remove "eid:" prefix to get actual EID
        actual_eid = eid_term[4:]
        
        # Basic validation - check if it looks like a valid EID
        if not self._is_valid_eid_format(actual_eid):
            return ResolutionResult(
                term=eid_term,
                resolution_type=ResolutionType.NOT_FOUND,
                suggestion=f"Invalid EID format: {actual_eid}"
            )
        
        # Check if EID exists in database
        if self._eid_exists_in_database(actual_eid):
            return ResolutionResult(
                term=eid_term,
                resolution_type=ResolutionType.EXPLICIT,
                resolved_eid=actual_eid,
                confidence=1.0
            )
        else:
            # Well-formed EID that doesn't exist - auto-create it
            if self.auto_create:
                # Store in session cache
                self.session_cache[eid_term] = actual_eid
                
                return ResolutionResult(
                    term=eid_term,
                    resolution_type=ResolutionType.AUTO_CREATED,
                    resolved_eid=actual_eid,
                    confidence=1.0,
                    suggestion=f"Auto-created explicit EID: {actual_eid}"
                )
            else:
                return ResolutionResult(
                    term=eid_term,
                    resolution_type=ResolutionType.NOT_FOUND,
                    suggestion=f"EID not found in database: {actual_eid}"
                )
    
    def _resolve_natural_term(self, term: str, entity_role: str) -> ResolutionResult:
        """Handle natural language term resolution with auto-creation"""
        
        # Search for existing candidates
        candidates = self._find_eid_candidates(term)
        
        if len(candidates) == 0:
            # No existing entities found
            if self.auto_create:
                # Auto-create new EID
                new_eid = self._generate_eid_for_term(term, entity_role)
                if new_eid:
                    # Store in session cache
                    self.session_cache[term] = new_eid
                    
                    return ResolutionResult(
                        term=term,
                        resolution_type=ResolutionType.AUTO_CREATED,
                        resolved_eid=new_eid,
                        confidence=1.0,
                        suggestion=f"Auto-created EID: {new_eid}"
                    )
                else:
                    return ResolutionResult(
                        term=term,
                        resolution_type=ResolutionType.NOT_FOUND,
                        suggestion=f"Failed to auto-create EID for: {term}"
                    )
            else:
                # Suggest creation but don't auto-create
                suggested_eid = self._generate_eid_for_term(term, entity_role)
                return ResolutionResult(
                    term=term,
                    resolution_type=ResolutionType.NOT_FOUND,
                    suggestion=f"Create new EID: {suggested_eid}"
                )
        
        elif len(candidates) == 1:
            # Unique match found
            candidate = candidates[0]
            self.session_cache[term] = candidate.eid
            return ResolutionResult(
                term=term,
                resolution_type=ResolutionType.UNIQUE_MATCH,
                resolved_eid=candidate.eid,
                confidence=candidate.confidence
            )
        
        else:
            # Multiple candidates - disambiguation needed
            return ResolutionResult(
                term=term,
                resolution_type=ResolutionType.AMBIGUOUS,
                candidates=candidates[:5]  # Limit to top 5
            )
    
    def _find_eid_candidates(self, term: str) -> List[EIDCandidate]:
        """Find EID candidates for natural language term"""
        candidates = []
        
        if not LABDB_AVAILABLE or not self.db_manager.is_connected:
            return candidates
        
        try:
            # Search for entities containing the term (simplified implementation)
            # In a full implementation, this would use more sophisticated matching
            
            # For now, create a simple candidate based on term similarity
            # This is a placeholder - real implementation would query LabDb
            sanitized_term = self._sanitize_term(term)
            suggested_eid = f"entity:{sanitized_term}"
            
            # Check if this suggested EID already exists
            if self._eid_exists_in_database(suggested_eid):
                candidates.append(EIDCandidate(
                    eid=suggested_eid,
                    description=f"Entity named '{term}'",
                    entity_type="entity",
                    confidence=0.9,
                    context={"match_type": "exact_name"}
                ))
            
        except Exception as e:
            self.logger.warning(f"Error searching for EID candidates: {e}")
        
        return candidates
    
    def _is_valid_eid_format(self, eid: str) -> bool:
        """Check if EID has valid format"""
        # Basic format validation - contains at least one colon and looks reasonable
        if ":" not in eid:
            return False
        parts = eid.split(":", 1)
        if len(parts) != 2:
            return False
        prefix, suffix = parts
        # Basic sanity checks
        if not prefix or not suffix:
            return False
        if not re.match(r"^[a-zA-Z_][a-zA-Z0-9_]*$", prefix):
            return False
        return True
    
    def _eid_exists_in_database(self, eid: str) -> bool:
        """Check if EID exists in the database"""
        if not LABDB_AVAILABLE or not self.db_manager.is_connected:
            return False
        
        try:
            # Query LabDb to check if entity exists
            # This is a placeholder - real implementation would use LabDb API
            result = self.db_manager.query_entity(eid)
            return result is not None
        except Exception as e:
            self.logger.debug(f"Error checking EID existence: {e}")
            return False
    
    def _generate_eid_for_term(self, term: str, entity_role: str) -> Optional[str]:
        """Generate a new EID for the given term"""
        try:
            sanitized = self._sanitize_term(term)
            if not sanitized:
                return None
            
            # Simple EID generation - could be made more sophisticated
            if entity_role == "predicate":
                eid = f"relation:{sanitized}"
            else:
                eid = f"entity:{sanitized}"
            
            # Ensure uniqueness by adding suffix if needed
            base_eid = eid
            counter = 1
            while self._eid_exists_in_database(eid):
                eid = f"{base_eid}_{counter}"
                counter += 1
                if counter > 100:  # Safety limit
                    return None
            
            return eid
            
        except Exception as e:
            self.logger.error(f"Error generating EID for term '{term}': {e}")
            return None
    
    def _sanitize_term(self, term: str) -> str:
        """Sanitize term for use in EID"""
        # Convert to lowercase, replace spaces with underscores, remove special chars
        sanitized = re.sub(r"[^a-zA-Z0-9_\s]", "", term.lower())
        sanitized = re.sub(r"\s+", "_", sanitized.strip())
        return sanitized


class AddTripleTool:
    """
    Add triple tool with conversational EID resolution.
    
    Supports both natural language and explicit EID modes:
    - add_triple("rose_quartz", "has_color", "pink") 
    - add_triple("rose_quartz", "has_color", "eid:pink_003")
    """
    
    def __init__(self, database_manager):
        """Initialize add triple tool with database manager"""
        self.db_manager = database_manager
        self.resolver = TriadicEIDResolver(database_manager)
        self.logger = logging.getLogger(__name__)
    
    async def add_triple(self, subject: str, predicate: str, object: str, validate: bool = True) -> Dict[str, Any]:
        """
        Add a triple to the triadic consciousness database with dual-mode EID resolution.
        
        Args:
            subject: Subject term (natural language or eid:explicit_id)
            predicate: Predicate/relationship term  
            object: Object term
            validate: Whether to validate the triple before adding
            
        Returns:
            Dict with operation result, including resolution feedback and any disambiguation needs
        """
        try:
            # Check database connection
            if not self.db_manager.is_connected:
                return {
                    "status": "error",
                    "message": "No database connection available",
                    "error_type": "connection_error"
                }
            
            # Resolve all three terms
            subject_result, predicate_result, object_result = self.resolver.resolve_triple_terms(
                subject, predicate, object
            )
            
            # Check for disambiguation needs
            ambiguous_terms = []
            if subject_result.resolution_type == ResolutionType.AMBIGUOUS:
                ambiguous_terms.append(("subject", subject, subject_result.candidates))
            if predicate_result.resolution_type == ResolutionType.AMBIGUOUS:
                ambiguous_terms.append(("predicate", predicate, predicate_result.candidates))
            if object_result.resolution_type == ResolutionType.AMBIGUOUS:
                ambiguous_terms.append(("object", object, object_result.candidates))
            
            if ambiguous_terms:
                return self._create_disambiguation_response(ambiguous_terms)
            
            # Check for missing terms
            missing_terms = []
            if subject_result.resolution_type == ResolutionType.NOT_FOUND:
                missing_terms.append(("subject", subject, subject_result.suggestion))
            if predicate_result.resolution_type == ResolutionType.NOT_FOUND:
                missing_terms.append(("predicate", predicate, predicate_result.suggestion))
            if object_result.resolution_type == ResolutionType.NOT_FOUND:
                missing_terms.append(("object", object, object_result.suggestion))
            
            if missing_terms:
                return self._create_missing_terms_response(missing_terms)
            
            # All terms resolved - proceed with triple addition
            resolved_subject = subject_result.resolved_eid
            resolved_predicate = predicate_result.resolved_eid
            resolved_object = object_result.resolved_eid
            
            # Add the triple to the database
            success = await self._add_triple_to_database(
                resolved_subject, resolved_predicate, resolved_object, validate
            )
            
            if success:
                return {
                    "status": "success",
                    "operation": "add_triple",
                    "original_triple": {
                        "subject": subject,
                        "predicate": predicate, 
                        "object": object
                    },
                    "resolved_triple": {
                        "subject": resolved_subject,
                        "predicate": resolved_predicate,
                        "object": resolved_object
                    },
                    "resolution_feedback": {
                        "subject": f'"{subject}" → {resolved_subject}' + self._resolution_type_annotation(subject_result),
                        "predicate": f'"{predicate}" → {resolved_predicate}' + self._resolution_type_annotation(predicate_result),
                        "object": f'"{object}" → {resolved_object}' + self._resolution_type_annotation(object_result)
                    },
                    "message": f"Successfully added triple: ({subject}, {predicate}, {object})",
                    "validated": validate
                }
            else:
                return {
                    "status": "error",
                    "message": "Failed to add triple to database",
                    "error_type": "database_error"
                }
                
        except Exception as e:
            self.logger.error(f"Error in add_triple: {e}")
            return {
                "status": "error", 
                "message": f"Unexpected error: {str(e)}",
                "error_type": "internal_error"
            }
    
    def _resolution_type_annotation(self, result: ResolutionResult) -> str:
        """Create annotation for resolution type"""
        if result.resolution_type == ResolutionType.EXPLICIT:
            return " (explicit EID)"
        elif result.resolution_type == ResolutionType.AUTO_CREATED:
            return " (auto-created)"
        elif result.resolution_type == ResolutionType.UNIQUE_MATCH:
            return " (found existing)"
        else:
            return ""
    
    def _create_disambiguation_response(self, ambiguous_terms: List[Tuple[str, str, List]]) -> Dict[str, Any]:
        """Create response for disambiguation needs"""
        disambiguations = {}
        for position, term, candidates in ambiguous_terms:
            disambiguations[position] = {
                "term": term,
                "candidates": [
                    {
                        "eid": candidate.eid,
                        "description": candidate.description,
                        "entity_type": candidate.entity_type,
                        "confidence": candidate.confidence
                    }
                    for candidate in candidates
                ]
            }
        
        return {
            "status": "disambiguation_needed",
            "operation": "add_triple",
            "message": "Multiple EID candidates found. Please clarify which entities you mean.",
            "ambiguous_terms": disambiguations,
            "next_action": "Specify which entity you mean for each ambiguous term"
        }
    
    def _create_missing_terms_response(self, missing_terms: List[Tuple[str, str, str]]) -> Dict[str, Any]:
        """Create response for missing terms"""
        suggestions = {}
        for position, term, suggestion in missing_terms:
            suggestions[position] = {
                "term": term,
                "suggestion": suggestion,
                "action": "create_new_entity"
            }
        
        return {
            "status": "entities_not_found",
            "operation": "add_triple", 
            "message": "Some terms were not found in the database. New entities can be created.",
            "missing_terms": suggestions,
            "next_action": "Confirm creation of new entities or provide explicit EIDs"
        }
    
    async def _add_triple_to_database(self, subject: str, predicate: str, obj: str, validate: bool) -> bool:
        """Add the resolved triple to the database"""
        try:
            if not LABDB_AVAILABLE:
                self.logger.warning("LabDb not available - cannot add triple")
                return False
            
            # Use database manager to add the triple
            # DEBUG: Check database health BEFORE adding triple
            try:
                health_before = await self.db_manager.health_check()
                self.logger.info(f"HEALTH CHECK BEFORE add_triple: {health_before}")
                print(f"DEBUG: HEALTH CHECK BEFORE add_triple: {health_before}", file=sys.stderr)
            except Exception as e:
                self.logger.error(f"Health check before failed: {e}")
                print(f"DEBUG: Health check before failed: {e}", file=sys.stderr)
            
            result = self.db_manager.add_triple(subject, predicate, obj, validate=validate)
            
            # DEBUG: Check database health AFTER adding triple  
            try:
                health_after = await self.db_manager.health_check()
                self.logger.info(f"HEALTH CHECK AFTER add_triple: {health_after}")
                print(f"DEBUG: HEALTH CHECK AFTER add_triple: {health_after}", file=sys.stderr)
            except Exception as e:
                self.logger.error(f"Health check after failed: {e}")
                print(f"DEBUG: Health check after failed: {e}", file=sys.stderr)
            
            return result is not None
            
        except Exception as e:
            self.logger.error(f"Error adding triple to database: {e}")
            return False
