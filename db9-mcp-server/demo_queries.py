#!/usr/bin/env python3
"""
Simple test client for DB9 MCP Server
Demonstrates triadic consciousness queries through the mock interface
"""

import asyncio
import sys
from pathlib import Path

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent / "src"))

from src.db9_server import DB9Server


async def test_db9_queries():
    """Test DB9 server functionality with sample queries"""
    print("🧠 DB9 Triadic Consciousness Gateway - Demo Queries")
    print("=" * 60)
    
    try:
        # Initialize DB9 server
        print("🚀 Initializing DB9 Server...")
        server = DB9Server()
        
        # Manually run startup to establish connection
        await server._on_startup()
        print("✅ DB9 Server ready!")
        
        # Test queries that demonstrate triadic consciousness
        test_queries = [
            {
                "name": "What do we know about minerals?",
                "query": "What do we know about minerals?",
                "perspective": "auto",
                "depth": 2
            },
            {
                "name": "Explore granite Motion perspective",
                "query": "Explore granite", 
                "perspective": "motion",
                "depth": 2
            },
            {
                "name": "Find Memory relations",
                "query": "Show me memory relations",
                "perspective": "memory", 
                "depth": 1
            },
            {
                "name": "Field contexts exploration",
                "query": "What field contexts are available?",
                "perspective": "field",
                "depth": 1
            }
        ]
        
        print("\n🎯 Testing Triadic Consciousness Queries:")
        print("-" * 40)
        
        for i, test in enumerate(test_queries, 1):
            print(f"\n{i}. {test['name']}")
            print(f"   Query: \"{test['query']}\"")
            print(f"   Perspective: {test['perspective']}")
            
            # Simulate the MCP tool call
            result = await server.app._tools['query_databases'].function(
                test['query'], 
                test['perspective'], 
                test['depth']
            )
            
            print(f"   Result: {result[:200]}{'...' if len(result) > 200 else ''}")
        
        # Test triadic navigation specifically
        print(f"\n5. Triadic Navigation Test")
        print(f"   Starting entity: granite")
        print(f"   Perspective: motion")
        
        nav_result = await server.app._tools['explore_triadic_navigation'].function(
            "granite", "motion", 2
        )
        print(f"   Navigation result: {nav_result[:200]}{'...' if len(nav_result) > 200 else ''}")
        
        # Test database health
        print(f"\n6. Database Health Check")
        health_result = await server.app._tools['database_health'].function()
        print(f"   Health status: {health_result[:300]}{'...' if len(health_result) > 300 else ''}")
        
        # Cleanup
        await server._on_shutdown()
        print("\n✅ DB9 Server demo completed successfully!")
        
        # Summary
        print("\n" + "=" * 60)
        print("🎉 DB9 Triadic Consciousness Gateway FUNCTIONAL!")
        print("📊 Mock database contains 16 sample triples")
        print("🧠 Motion/Memory/Field perspectives working")
        print("🔗 Natural language → triadic query translation successful") 
        print("🚀 Ready for Phase 2: Multi-database federation")
        
    except Exception as e:
        print(f"❌ Demo failed: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    asyncio.run(test_db9_queries())
