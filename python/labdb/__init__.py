"""
LabDb - Triadic Consciousness Database

A nonostore implementation that embodies त्रित्रयम् (triadic consciousness) principles
through cube architecture. Extends traditional hexastore with vocabulary discovery
for complete ontological awareness.

Core Classes:
    NonoStore: Main database interface with 9-index storage
    TriadicQuery: Conscious navigation through Motion/Memory/Field perspectives
"""

__version__ = "0.1.0"
__author__ = "Lab Team"

# Triadic perspective constants for clarity
class Perspective:
    """Triadic consciousness perspectives for navigation"""
    MOTION = "Motion"    # स्पन्द (spanda) - Subject-driven reality
    MEMORY = "Memory"    # स्मृति (smriti) - Predicate-driven relationships  
    FIELD = "Field"      # क्षेत्र (kshetra) - Object-driven contexts

# Try to import C++ bindings, fall back gracefully if not built
try:
    from .pylabdb import *
    from .pylabdb import (
        NonoStore, Triple, Stats,
        TriadicQuery, TriadicResult, TriadicStats, VocabularyBoundary,
        perspective_name, perspective_sanskrit, optimal_perspective
    )
    
    # Import C++ Perspective enum and create alias
    from .pylabdb import Perspective as CPPPerspective
    
    # Create a unified Perspective class that includes both Python constants and C++ enum
    class Perspective:
        """Triadic consciousness perspectives for navigation"""
        MOTION = "Motion"    # स्पन्द (spanda) - Subject-driven reality
        MEMORY = "Memory"    # स्मृति (smriti) - Predicate-driven relationships  
        FIELD = "Field"      # क्षेत्र (kshetra) - Object-driven contexts
        
        # C++ enum values
        Motion = CPPPerspective.Motion
        Memory = CPPPerspective.Memory 
        Field = CPPPerspective.Field
    
    __all__ = [
        'NonoStore', 'Triple', 'Stats',
        'TriadicQuery', 'TriadicResult', 'TriadicStats', 'VocabularyBoundary',
        'Perspective',
        'perspective_name', 'perspective_sanskrit', 'optimal_perspective',
        'EnhancedNonoStore', 'EnhancedTriadicResult', 'EnhancedTriadicResults'
    ]
    
    _bindings_available = True
    
    # Import enhanced classes
    from .enhanced import EnhancedNonoStore, EnhancedTriadicResult, EnhancedTriadicResults
    
except ImportError:
    # C++ bindings not built yet
    __all__ = ['Perspective']
    _bindings_available = False
    
    def _not_available(*args, **kwargs):
        raise RuntimeError(
            "LabDb C++ bindings not available. "
            "Please build with: cmake -DBUILD_PYTHON_BINDINGS=ON .. && make pylabdb"
        )
    
    # Provide placeholder classes that give helpful error messages
    NonoStore = _not_available
    Triple = _not_available
    Stats = _not_available
    TriadicQuery = _not_available
    TriadicResult = _not_available
    TriadicStats = _not_available
    VocabularyBoundary = _not_available
    perspective_name = _not_available
    perspective_sanskrit = _not_available
    optimal_perspective = _not_available
    EnhancedNonoStore = _not_available
    EnhancedTriadicResult = _not_available
    EnhancedTriadicResults = _not_available
