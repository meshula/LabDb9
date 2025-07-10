# Test Chains Database Documentation

## Database: test_chains.db9

This database contains test entities specifically designed to validate EID chain resolution functionality.

## Entity Structure

| EID   | Value        | Chain Type | Expected Resolution |
|-------|--------------|------------|-------------------|
| eid:1 | "DirectValue" | None       | "DirectValue"     |
| eid:4 | "eid:1"      | 2-level    | "DirectValue"     |
| eid:5 | "ChainEnd"   | None       | "ChainEnd"        |
| eid:6 | "eid:3"      | Broken     | (should fail)     |
| eid:7 | "eid:5"      | 2-level    | "ChainEnd"        |

## Chain Resolution Examples

### Simple Direct Resolution
- `eid:1` → `"DirectValue"` ✅ (no chain)
- `eid:5` → `"ChainEnd"` ✅ (no chain)

### Two-Level Chains
- `eid:4` → `"eid:1"` → `"DirectValue"` ✅
- `eid:7` → `"eid:5"` → `"ChainEnd"` ✅

### Broken Chain (Error Testing)
- `eid:6` → `"eid:3"` → ❌ (eid:3 doesn't exist)

## Usage in Tests

This database is designed to test:
1. **Basic enhanced API functionality** - entities resolve correctly
2. **EID chain resolution** - multi-level indirection works
3. **Error handling** - broken chains are handled gracefully
4. **Performance** - chain resolution doesn't significantly impact performance

## Database Creation Commands

```lisp
(create-database :path "test_chains.db9")
(add-entity :dbid "db1" :value "DirectValue")    ; -> eid:1
(add-entity :dbid "db1" :value "eid:1")          ; -> eid:4
(add-entity :dbid "db1" :value "ChainEnd")       ; -> eid:5
(add-entity :dbid "db1" :value "eid:3")          ; -> eid:6 (broken)
(add-entity :dbid "db1" :value "eid:5")          ; -> eid:7
```

## Expected Test Results

When using the enhanced API:
- `get-entity-enhanced eid:1` → `{"eid":"eid:1","value":"DirectValue","type":"entity"}`
- `get-entity-enhanced eid:4` → `{"eid":"eid:4","value":"DirectValue","type":"entity"}`
- `get-entity-enhanced eid:7` → `{"eid":"eid:7","value":"ChainEnd","type":"entity"}`
