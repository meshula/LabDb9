#!/usr/bin/env python3
"""
LabDb MCP Server - Test Version
"""

import sys
import json
from typing import List

from fastmcp import FastMCP

app = FastMCP("LabDb db9 S-Expression Gateway")

@app.tool
def db9(commands: str) -> str:
    """Execute db9 S-expression commands."""
    try:
        command_list = json.loads(commands)
        return json.dumps({
            "status": "success", 
            "message": f"Would execute {len(command_list)} commands: {command_list}",
            "note": "This is a test version - LabDb not actually called"
        })
    except Exception as e:
        return json.dumps({"status": "error", "error": str(e)})

@app.tool
def db9_readme() -> str:
    """Get db9 specification."""
    return "LabDb Test Server - C extension bypassed for testing"

if __name__ == "__main__":
    app.run()
