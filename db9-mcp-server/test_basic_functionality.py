#!/usr/bin/env python3
"""
Basic functionality test for DB9 MCP Server Phase 1
Tests database connection, configuration loading, and basic query tools
"""

import asyncio
import logging
import sys
from pathlib import Path

# Add src to path for imports
sys.path.insert(0, str(Path(__file__).parent / "src"))

from src.database_manager import DatabaseManager
from src.tools.query_tools import QueryTools


async def test_database_connection():
    """Test basic database connection (with mock fallback)"""
    print("🔗 Testing Database Connection...")
    
    # Load default config
    config_path = Path("db9-mcp-server/config/default_config.yaml")
    if not config_path.exists():
        print(f"❌ Config file not found: {config_path}")
        return False
    
    import yaml
    with open(config_path) as f:
        config = yaml.safe_load(f)
    
    # Test database connection (real or mock)
    db_manager = DatabaseManager(config)
    
    try:
        connected = await db_manager.connect()
        if connected:
            print("✅ Database connection successful (using mock interface)")
            
            # Test health check
            health = await db_manager.health_check()
            print(f"✅ Database health: {health['status']}")
            
            if health['status'] == 'healthy':
                stats = health['stats']
                print(f"📊 Database stats:")
                print(f"  • Triples: {stats['triple_count']}")
                print(f"  • Motion entities: {stats['motion_entities']}")
                print(f"  • Memory relations: {stats['memory_relations']}")
                print(f"  • Field contexts: {stats['field_contexts']}")
                
                # Test triadic query functionality
                triadic_query = db_manager.get_triadic_query()
                if triadic_query:
                    print("✅ Triadic query interface available")
                    
                    # Test a simple query
                    result = triadic_query.motion_from("granite")
                    if result and hasattr(result, 'triples'):
                        print(f"✅ Motion query test: Found {len(result.triples)} granite relationships")
                    else:
                        print("⚠️ Motion query returned no results")
            
            await db_manager.disconnect()
            return True
        else:
            print("❌ Database connection failed")
            return False
            
    except Exception as e:
        print(f"❌ Database connection error: {e}")
        return False


def test_query_tools():
    """Test natural language query parsing"""
    print("\n🧠 Testing Query Tools...")
    
    query_tools = QueryTools()
    
    test_queries = [
        "What do we know about minerals?",
        "Explore granite from memory perspective", 
        "Show me field contexts for rocks",
        "What entities are Motion-focused?",
        "Navigate triadic relationships depth 3"
    ]
    
    for query in test_queries:
        hints = query_tools.parse_natural_query(query)
        print(f"✅ Query: '{query}'")
        print(f"   → Entities: {hints.entities}")
        print(f"   → Perspective: {hints.perspective}")
        print(f"   → Depth: {hints.depth}")
        print(f"   → Type: {hints.query_type}")
    
    return True


def test_configuration():
    """Test configuration loading"""
    print("\n⚙️  Testing Configuration...")
    
    config_path = Path("db9-mcp-server/config/default_config.yaml")
    if not config_path.exists():
        print(f"❌ Config file not found: {config_path}")
        return False
    
    try:
        import yaml
        with open(config_path) as f:
            config = yaml.safe_load(f)
        
        # Validate essential config sections
        required_sections = ["database", "server", "features"]
        for section in required_sections:
            if section not in config:
                print(f"❌ Missing config section: {section}")
                return False
        
        print("✅ Configuration loaded successfully")
        print(f"✅ Database path: {config['database']['path']}")
        print(f"✅ Server name: {config['server']['name']}")
        
        return True
        
    except Exception as e:
        print(f"❌ Configuration error: {e}")
        return False


async def test_fastmcp_import():
    """Test FastMCP availability"""
    print("\n🚀 Testing FastMCP Integration...")
    
    try:
        from fastmcp import FastMCP
        print("✅ FastMCP import successful")
        
        # Test basic app creation
        app = FastMCP("Test App")
        print("✅ FastMCP app creation successful")
        
        return True
        
    except ImportError:
        print("❌ FastMCP not available")
        print("💡 Install with: pip install fastmcp")
        return False
    except Exception as e:
        print(f"❌ FastMCP error: {e}")
        return False


async def main():
    """Run all tests"""
    print("🧪 DB9 MCP Server - Basic Functionality Tests")
    print("=" * 50)
    
    tests = [
        ("Configuration", test_configuration),
        ("FastMCP Integration", test_fastmcp_import),
        ("Query Tools", test_query_tools),
        ("Database Connection", test_database_connection),
    ]
    
    results = []
    
    for test_name, test_func in tests:
        print(f"\n📋 Running: {test_name}")
        print("-" * 30)
        
        try:
            if asyncio.iscoroutinefunction(test_func):
                result = await test_func()
            else:
                result = test_func()
            
            results.append((test_name, result))
            
        except Exception as e:
            print(f"❌ Test '{test_name}' failed with exception: {e}")
            results.append((test_name, False))
    
    # Summary
    print("\n" + "=" * 50)
    print("📊 Test Results Summary:")
    
    passed = 0
    for test_name, result in results:
        status = "✅ PASSED" if result else "❌ FAILED"
        print(f"  {status}: {test_name}")
        if result:
            passed += 1
    
    print(f"\n🎯 Overall: {passed}/{len(results)} tests passed")
    
    if passed == len(results):
        print("🎉 All tests passed! DB9 Phase 1 foundation is ready.")
        print("💡 Next step: Run 'python -m src.db9_server' to start the server")
    else:
        print("⚠️  Some tests failed. Please check the issues above.")
        
        # Provide specific guidance
        if not any(name == "FastMCP Integration" and result for name, result in results):
            print("💡 Install FastMCP: pip install fastmcp")
        
        if not any(name == "Database Connection" and result for name, result in results):
            print("💡 Check LabDb installation: pip install -e ../python/")
            print("💡 Verify database path in config/default_config.yaml")


if __name__ == "__main__":
    # Setup basic logging
    logging.basicConfig(level=logging.INFO)
    
    # Run tests
    asyncio.run(main())
