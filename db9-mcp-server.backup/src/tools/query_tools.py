"""
Basic Query Tools for DB9 MCP Server Phase 1
Natural language → triadic query translation (keyword-based)
"""

import logging
import re
from typing import List, Optional, Dict, Any, Tuple
from dataclasses import dataclass

@dataclass
class QueryHints:
    """Extracted hints from natural language query"""
    entities: List[str]           # Potential entity names
    perspective: str              # "motion", "memory", "field", or "auto"
    depth: int                   # Navigation depth
    domain_hints: List[str]      # Domain keywords
    query_type: str              # "explore", "find", "what", "show"


class QueryTools:
    """
    Basic natural language query processing for Phase 1
    
    Implements keyword-based query understanding without LLM dependency.
    Future phases will add sophisticated LLM-powered query expansion.
    """
    
    def __init__(self):
        self.logger = logging.getLogger(__name__)
        
        # Perspective keywords for triadic consciousness navigation
        self.perspective_keywords = {
            "motion": ["motion", "subject", "entity", "entities", "what", "who", "express", "action"],
            "memory": ["memory", "relation", "relationship", "connect", "link", "how", "relates"],
            "field": ["field", "context", "object", "ground", "where", "place", "setting"]
        }
        
        # Query type patterns
        self.query_patterns = {
            "explore": r"\b(explore|navigate|traverse|walk|follow)\b",
            "find": r"\b(find|search|look|discover|locate)\b", 
            "what": r"\b(what|which|tell|show|describe)\b",
            "count": r"\b(count|how many|number)\b"
        }
        
        # Domain vocabulary hints
        self.domain_keywords = {
            "geology": ["mineral", "rock", "stone", "granite", "quartz", "crystal"],
            "geometry": ["circle", "line", "triangle", "angle", "construction", "proof"],
            "materials": ["material", "texture", "color", "property", "surface"],
            "general": ["entity", "thing", "concept", "item", "object"]
        }
    
    def parse_natural_query(self, query: str) -> QueryHints:
        """
        Parse natural language query into triadic consciousness hints
        
        Args:
            query: Natural language query string
            
        Returns:
            QueryHints with extracted information for triadic navigation
        """
        query_lower = query.lower().strip()
        
        # Extract entities (simple heuristic - look for quoted terms and capitalized words)
        entities = self._extract_entities(query)
        
        # Determine perspective preference
        perspective = self._determine_perspective(query_lower)
        
        # Extract depth hints
        depth = self._extract_depth(query_lower)
        
        # Identify domain hints
        domain_hints = self._identify_domains(query_lower)
        
        # Classify query type
        query_type = self._classify_query_type(query_lower)
        
        hints = QueryHints(
            entities=entities,
            perspective=perspective,
            depth=depth,
            domain_hints=domain_hints,
            query_type=query_type
        )
        
        self.logger.debug(f"Parsed query '{query}' → {hints}")
        return hints
    
    def _extract_entities(self, query: str) -> List[str]:
        """Extract potential entity names from query"""
        entities = []
        
        # Look for quoted terms
        quoted_pattern = r'"([^"]+)"'
        quoted_matches = re.findall(quoted_pattern, query)
        entities.extend(quoted_matches)
        
        # Look for capitalized words (potential proper nouns)
        capitalized_pattern = r'\b[A-Z][a-z]+\b'
        capitalized_matches = re.findall(capitalized_pattern, query)
        entities.extend(capitalized_matches)
        
        # Common entity keywords in queries
        entity_patterns = [
            r'\babout\s+(\w+)',
            r'\bfor\s+(\w+)',
            r'\bof\s+(\w+)',
            r'\bwith\s+(\w+)'
        ]
        
        for pattern in entity_patterns:
            matches = re.findall(pattern, query.lower())
            entities.extend(matches)
        
        # Remove duplicates and common words
        stopwords = {'the', 'and', 'or', 'but', 'with', 'from', 'about', 'for', 'of'}
        entities = [e for e in set(entities) if e.lower() not in stopwords]
        
        return entities
    
    def _determine_perspective(self, query: str) -> str:
        """Determine preferred triadic perspective based on keywords"""
        perspective_scores = {"motion": 0, "memory": 0, "field": 0}
        
        for perspective, keywords in self.perspective_keywords.items():
            for keyword in keywords:
                if keyword in query:
                    perspective_scores[perspective] += 1
        
        # Return perspective with highest score, or "auto" if tied
        max_score = max(perspective_scores.values())
        if max_score == 0:
            return "auto"
        
        # Find perspective(s) with max score
        max_perspectives = [p for p, s in perspective_scores.items() if s == max_score]
        
        if len(max_perspectives) == 1:
            return max_perspectives[0]
        else:
            return "auto"  # Multiple perspectives equally likely
    
    def _extract_depth(self, query: str) -> int:
        """Extract navigation depth from query"""
        # Look for explicit depth mentions
        depth_pattern = r'\bdepth\s+(\d+)'
        match = re.search(depth_pattern, query)
        if match:
            return int(match.group(1))
        
        # Look for level mentions
        level_pattern = r'\blevel\s+(\d+)'
        match = re.search(level_pattern, query)
        if match:
            return int(match.group(1))
        
        # Infer depth from query complexity
        if "deep" in query or "comprehensive" in query:
            return 3
        elif "surface" in query or "brief" in query:
            return 1
        else:
            return 2  # Default depth
    
    def _identify_domains(self, query: str) -> List[str]:
        """Identify domain hints from vocabulary"""
        domains = []
        
        for domain, keywords in self.domain_keywords.items():
            for keyword in keywords:
                if keyword in query:
                    domains.append(domain)
                    break  # Only add domain once
        
        return list(set(domains))  # Remove duplicates
    
    def _classify_query_type(self, query: str) -> str:
        """Classify the type of query being asked"""
        for query_type, pattern in self.query_patterns.items():
            if re.search(pattern, query):
                return query_type
        
        return "what"  # Default query type
    
    def format_triadic_response(self, results: Dict[str, Any], perspective: str) -> str:
        """
        Format triadic query results for natural language response
        
        Args:
            results: Results from triadic consciousness query
            perspective: The perspective used ("motion", "memory", "field")
            
        Returns:
            Formatted natural language response
        """
        if not results:
            return "No results found in the triadic consciousness database."
        
        response_parts = []
        
        # Add perspective-aware header
        perspective_descriptions = {
            "motion": "Motion (स्पन्द) - Dynamic expressions and actions",
            "memory": "Memory (स्मृति) - Relational connections and patterns", 
            "field": "Field (क्षेत्र) - Contextual groundings and manifestations"
        }
        
        if perspective in perspective_descriptions:
            response_parts.append(f"**{perspective_descriptions[perspective]}:**")
        
        # Format results based on type
        if "triples" in results:
            triples = results["triples"]
            response_parts.append(f"Found {len(triples)} triadic relationships:")
            
            for triple in triples[:10]:  # Limit to first 10 for readability
                response_parts.append(f"  • {triple}")
            
            if len(triples) > 10:
                response_parts.append(f"  ... and {len(triples) - 10} more")
        
        if "vocabulary" in results:
            vocab = results["vocabulary"]
            response_parts.append(f"Vocabulary: {len(vocab)} unique terms")
        
        if "stats" in results:
            stats = results["stats"]
            response_parts.append(f"Statistics: {stats}")
        
        return "\n".join(response_parts)
