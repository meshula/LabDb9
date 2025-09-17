"""
LabDb - Auto-Reflexive Database with Memex functionality

Minimal Python package providing direct access to LabDb's C++ S-expression interface.
No abstraction layers, no complex wrappers - just direct access to the proven C++ implementation.

Core Functions:
    execute_db9_command(command): Execute single S-expression command
    execute_db9_commands(commands): Execute multiple S-expression commands  
    get_db9_specification(): Get complete db9 usage specification
    get_db9_available_verbs(): Get list of available verbs

Example:
    import labdb
    
    # Get help
    print(labdb.get_db9_specification())
    
    # Execute commands
    result = labdb.execute_db9_command('(stats)')
    print(result)  # JSON response
"""

__version__ = "0.2.0"
__author__ = "Lab Team"

# Import all functions from the C extension
from .labdb import (
    execute_db9_command,
    execute_db9_commands, 
    get_db9_specification,
    get_db9_available_verbs
)

__all__ = [
    'execute_db9_command',
    'execute_db9_commands',
    'get_db9_specification', 
    'get_db9_available_verbs'
]
