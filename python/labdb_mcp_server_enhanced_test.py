#!/usr/bin/env python3
"""
LabDb MCP Server - Enhanced Diagnostics
"""
import sys
import json
import os
import random
from pathlib import Path
import importlib.util

# Enhanced logging
log_file = open("/tmp/labdb_mcp_debug.log", "a")
log_file.write(f"\n--- Enhanced Diagnostics: PID={os.getpid()} ---\n")

def log_and_print(msg):
    log_file.write(msg + "\n")
    log_file.flush()
    print(msg, file=sys.stderr)

# Test LabDb import explicitly
log_and_print("Testing LabDb import...")
try:
    import labdb
    log_and_print("✅ SUCCESS: import labdb worked!")
    log_and_print(f"LabDb verbs: {labdb.get_db9_available_verbs()}")
except ImportError as e:
    log_and_print(f"❌ FAILED: import labdb failed: {e}")
except Exception as e:
    log_and_print(f"❌ ERROR: labdb import caused exception: {e}")

from fastmcp import FastMCP
mcp = FastMCP(name="LabDb Test")

@mcp.tool
def test_labdb_import() -> str:
    """Test if LabDb can be imported inside tool."""
    try:
        import labdb
        verbs = labdb.get_db9_available_verbs()
        return f"SUCCESS: LabDb imported with {len(verbs)} verbs: {verbs}"
    except Exception as e:
        return f"FAILED: {str(e)}"

@mcp.tool
def roll_dice(n_dice: int) -> list[int]:
    """Roll dice for comparison."""
    return [random.randint(1, 6) for _ in range(n_dice)]

if __name__ == "__main__":
    log_and_print("About to call mcp.run()...")
    mcp.run()
