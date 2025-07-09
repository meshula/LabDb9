#!/bin/bash
# LabDb MCP Server Launch Script

# Set PYTHONPATH to include the LabDb Python directory
export PYTHONPATH="/Users/nporcino/bin/labdb:$PYTHONPATH"

# Launch the MCP server
exec "/Users/nporcino/miniconda3/envs/inception_mcp_dev/bin/python" "/Users/nporcino/bin/labdb/labdb-mcp-server.py" "$@"
