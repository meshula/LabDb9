#!/usr/bin/env python3
"""
LabDb MCP Server
Minimal FastMCP2 server providing db9 S-expression interface
"""

import sys
import json
import asyncio
from typing import List

# Import FastMCP2
try:
    from fastmcp import FastMCP
    FASTMCP_AVAILABLE = True
except ImportError:
    print("Error: FastMCP2 not available. Install with: pip install fastmcp", file=sys.stderr)
    sys.exit(1)

# Import LabDb
try:
    import labdb
    LABDB_AVAILABLE = True
except ImportError:
    print("Error: LabDb not available. Check PYTHONPATH and build.", file=sys.stderr)
    sys.exit(1)

# Create FastMCP2 app
app = FastMCP("LabDb db9 S-Expression Gateway")

@app.tool
def db9(commands: List[str]) -> str:
    """
    Execute db9 S-expression commands against triadic consciousness database.
    
    Args:
        commands: List of S-expression commands to execute
        
    Returns:
        JSON response with results and auto-reflexive metrics
        
    Examples:
        - db9(["(stats)"])
        - db9(["(add-triple granite contains quartz)"])
        - db9(["(find-triple granite)"])
    """
    print(f"DEBUG: db9 tool called with commands: {commands}", file=sys.stderr)
    try:
        if not commands:
            return json.dumps({
                "status": "error",
                "error_message": "No commands provided. Use db9_readme to learn the interface."
            })
        
        if len(commands) == 1:
            result = labdb.execute_db9_command(commands[0])
        else:
            result = labdb.execute_db9_commands(commands)
        
        return result
        
    except Exception as e:
        print(f"DEBUG: Exception in db9 tool: {e}", file=sys.stderr)
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
    print("DEBUG: db9_readme tool called", file=sys.stderr)
    try:
        return labdb.get_db9_specification()
    except Exception as e:
        print(f"DEBUG: Exception in db9_readme: {e}", file=sys.stderr)
        return f"Error getting specification: {str(e)}"

if __name__ == "__maixn__":
    print("Starting LabDb MCP Server...", file=sys.stderr)
    print(f"Available verbs: {labdb.get_db9_available_verbs()}", file=sys.stderr)
    print("DEBUG: About to call app.run()...", file=sys.stderr)
    try:
        time.sleep(1)  # crude delay to allow any background init to complete
        app.run()
    except Exception as e:
        print(f"ERROR in app.run(): {e}", file=sys.stderr)
        import traceback
        traceback.print_exc(file=sys.stderr)

import random
from fastmcp import FastMCP

mcp = FastMCP(name="Dice Roller")

@mcp.tool
def roll_dice(n_dice: int) -> list[int]:
    """Roll `n_dice` 6-sided dice and return the results."""
    return [random.randint(1, 6) for _ in range(n_dice)]

if __name__ == "__main__":
    mcp.run()