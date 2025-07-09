#!/usr/bin/env python3
"""
LabDb MCP Server - Dispatcher Initialization Test
"""
import sys
import json
import os
from fastmcp import FastMCP

mcp = FastMCP(name="LabDb Dispatcher Test")

@mcp.tool
def test_dispatcher_init() -> str:
    """Test various ways to initialize the LabDb dispatcher."""
    try:
        import labdb
        results = []
        
        # Test 1: Check initial state
        verbs = labdb.get_db9_available_verbs()
        results.append(f"Initial verbs: {len(verbs)} - {verbs}")
        
        # Test 2: Try to get global dispatcher (if there's a function)
        try:
            # Look for potential initialization functions
            dir_labdb = [attr for attr in dir(labdb) if 'init' in attr.lower() or 'dispatcher' in attr.lower()]
            results.append(f"Potential init functions: {dir_labdb}")
        except Exception as e:
            results.append(f"Error checking dir(labdb): {e}")
        
        # Test 3: Try executing a simple command to see what happens
        try:
            result = labdb.execute_db9_command("(stats)")
            results.append(f"execute_db9_command result: {result}")
        except Exception as e:
            results.append(f"execute_db9_command error: {e}")
            
        return "\n".join(results)
        
    except Exception as e:
        return f"Import or execution failed: {e}"

@mcp.tool
def list_labdb_functions() -> str:
    """List all available LabDb functions."""
    try:
        import labdb
        functions = [attr for attr in dir(labdb) if not attr.startswith('_')]
        return f"LabDb functions: {functions}"
    except Exception as e:
        return f"Error: {e}"

if __name__ == "__main__":
    mcp.run()
