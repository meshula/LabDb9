# 🤖 LLM-Ready Verb Checklist

## Criteria for "LLM-Ready":
- ✅ **Complete documentation** with usage examples
- ✅ **Parameter specifications** (what's required/optional)
- ✅ **Return format examples** (expected JSON structure)
- ✅ **Usage notes** explaining behavior
- ✅ **Consistent S-expression format**

---

## 📊 Verb Readiness Status

### Database Lifecycle
| Verb | Status | Documentation Quality |
|------|--------|----------------------|
| `create-database` | ✅ | **EXCELLENT** - Full usage example, parameters, returns, notes |
| `open-database` | ✅ | **EXCELLENT** - Complete documentation with examples |
| `close-database` | ✅ | **EXCELLENT** - Clear usage and return format |
| `database-health-check` | ✅ | **EXCELLENT** - Accurate docs with complete return field descriptions |
| `list-open-databases` | ✅ | **EXCELLENT** - Complete documentation with examples |

### Entity Operations
| Verb | Status | Documentation Quality |
|------|--------|----------------------|
| `add-entity` | ✅ | **EXCELLENT** - Complete with examples, parameters, returns |
| `get-entity` | ✅ | **EXCELLENT** - Full documentation and usage patterns |
| `find-entity` | ✅ | **EXCELLENT** - Clear wildcard examples and return format |
| `add-entities-bulk` | ✅ | **EXCELLENT** - Full documentation and usage patterns |

### Triple Operations  
| Verb | Status | Documentation Quality |
|------|--------|----------------------|
| `add-triple` |  ✅ | **EXCELLENT** - Full documentation and usage patterns |
| `find-triple` |  ✅ | **EXCELLENT** - Full documentation and usage patterns |
| `get-triple` | ❌ | **MINIMAL** - Only brief description |
| `remove-triple` | ❌ | **MINIMAL** - Only brief description |
| `add-triples-bulk` | ❌ | **MINIMAL** - Only brief description |

### Enhanced Verbs
| Verb | Status | Documentation Quality |
|------|--------|----------------------|
| `get-vocabulary-stats` | ✅ | **EXCELLENT** - Complete with rich examples and EID resolution |
| `find-entity-enhanced` | ❌ | **MINIMAL** - Only brief description |
| `get-entity-enhanced` | ❌ | **MINIMAL** - Only brief description |
| `find-triple-enhanced` | ❌ | **MINIMAL** - Only brief description |
| `get-triple-enhanced` | ❌ | **MINIMAL** - Only brief description |
| `find-relationships-enhanced` | ❌ | **MINIMAL** - Only brief description |

### Storage Layer Operations
| Verb | Status | Documentation Quality |
|------|--------|----------------------|
| `add-tid` | ❌ | **MINIMAL** - Only brief description |
| `find-tid` | ❌ | **MINIMAL** - Only brief description |
| `find-eid` | ❌ | **MINIMAL** - Only brief description |
| `add-triple-semantic` | ❌ | **MINIMAL** - Only brief description |

---

## 📈 Summary Stats

**✅ LLM-Ready: 8/24 verbs (33%)**
**❌ Needs Work: 16/24 verbs (67%)**

### ✅ **EXCELLENT Documentation (Ready for LLM use):**
- Database lifecycle: `create-database`, `open-database`, `close-database`
- Entity operations: `add-entity`, `get-entity`, `find-entity`  
- Analysis: `get-vocabulary-stats`
- Statistics: `database-health-check`

### ❌ **Needs Documentation Enhancement:**
- **All triple operations** (add, find, get, remove, bulk)
- **All enhanced verbs** (except vocabulary-stats)
- **All storage layer verbs** (add-tid, find-tid, etc.)
- **Bulk operations** (entities and triples)
- **Utility verbs** (list-open-databases)

---

## 🎯 Priority Recommendations

### **High Priority** (Core operations LLMs will use most):
1. `add-triple` - Fundamental operation
2. `find-triple` - Essential for queries  
3. `remove-triple` - Data management
4. `find-triple-enhanced` - Rich query results

### **Medium Priority** (Enhanced functionality):
5. `add-triples-bulk` - Performance operations
6. `find-entity-enhanced` - Rich entity queries
7. `get-triple-enhanced` - Detailed triple inspection

### **Low Priority** (Specialized operations):
8. Storage layer verbs (add-tid, find-tid, etc.)
9. `list-open-databases` - Utility function

---

## 📝 Documentation Template Needed

Each verb should follow the **excellent pattern** established by `get-vocabulary-stats`:

```markdown
### Verb: verb-name

Brief description of purpose and functionality.
Usage:
```lisp
(verb-name :param1 value1 :param2 value2 :dbid database-id)
;; Returns: expected structure
;; Notes: important behavior notes

;; Example:
(verb-name :param "example" :dbid db1)
;; Returns: {"key": "value", "status": "success"}
```

**Parameters:**
- `:param1` - Description of parameter
- `:dbid` - Database identifier

**Returns:**
- `key` - Description of return field
- `status` - Operation status

**Notes:**
- Important behavior details
- Usage patterns and tips
```

This would bring all verbs up to LLM-ready standards! 🚀