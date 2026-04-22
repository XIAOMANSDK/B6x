#!/usr/bin/env python3
"""
B6x MCP Server Launcher
========================

Convenient launcher script for running the B6x MCP server.
"""

import sys
from pathlib import Path

# Add src directory to Python path
src_path = Path(__file__).parent / "src"
sys.path.insert(0, str(src_path))

# Import and run main
if __name__ == "__main__":
    from src import main
    import asyncio

    try:
        asyncio.run(main.main())
    except KeyboardInterrupt:
        print("\nShutdown requested")
        sys.exit(0)
    except Exception as e:
        print(f"Fatal error: {e}", file=sys.stderr)
        sys.exit(1)
