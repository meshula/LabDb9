# Test Environment

This directory contains test databases and fixtures for LabDb unit tests.

## Purpose

- **Isolated test data**: Stable test databases that won't change over time
- **Reproducible tests**: Tests that don't depend on external data files
- **Version control**: Test databases can be committed to git for consistency

## Structure

```
testenv/
├── README.md           # This file
├── test_simple.db9     # Simple test database with basic entities
├── test_chains.db9     # Test database with EID chains for resolution testing
└── test_semantic.db9   # Test database with semantic relationships
```

## Usage

Tests should:
1. Use databases from this directory for stable, reproducible results
2. Create databases in `/tmp/` for temporary test operations
3. Not modify the databases in this directory during tests

## Future Work

- Add script to regenerate test databases
- Add more complex test scenarios
- Document expected data structure for each test database
