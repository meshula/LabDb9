#!/usr/bin/env python3
"""
LabDb MCP Server
Minimal FastMCP2 server providing db9 S-expression interface
"""

import sys
import json
from typing import List

# Import FastMCP2
try:
    from fastmcp import FastMCP
except ImportError:
    print("Error: FastMCP2 not available. Install with: pip install fastmcp", file=sys.stderr)
    sys.exit(1)

# Create FastMCP2 app
app = FastMCP("LabDb db9 S-Expression Gateway")

@app.tool
def db9(commands: str) -> str:
    """
    Execute db9 S-expression commands against triadic consciousness database.
    
    Args:
        commands: JSON string of S-expression commands to execute
        
    Returns:
        JSON response with results and auto-reflexive metrics
        
    Examples:
        - db9('["(stats)"]')
        - db9('["(add-triple granite contains quartz)"]')
        - db9('["(find-triple granite)"]')
    """
    try:
        # Lazy import LabDb only when needed
        import labdb
        
        # Parse the JSON string to get the actual list
        command_list = json.loads(commands)
        
        if not command_list:
            return json.dumps({
                "status": "error",
                "error_message": "No commands provided. Use db9_readme to learn the interface."
            })
        
        if len(command_list) == 1:
            result = labdb.execute_db9_command(command_list[0])
        else:
            result = labdb.execute_db9_commands(command_list)
        
        return result
        
    except ImportError:
        return json.dumps({
            "status": "error",
            "error_message": "LabDb not available. Check PYTHONPATH and build."
        })
    except Exception as e:
        return json.dumps({
            "status": "error",
            "error_message": f"db9 execution failed: {str(e)}"
        })

@app.tool
def db9_readme() -> str:
    """
    Get complete db9 specification and usage guide.
    
    Returns:
        Complete specification with all available verbs and examples
    """
    try:
        # Lazy import LabDb only when needed
        import labdb
        return labdb.get_db9_specification()
    except ImportError:
        return "Error: LabDb not available. Check PYTHONPATH and build."
    except Exception as e:
        return f"Error getting specification: {str(e)}"

if __name__ == "__main__":
    app.run()
