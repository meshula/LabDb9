# fio-preview: Unified Reflexive Awareness Specification

## Core Principle
File operations become conscious collaborative acts through preview-confirm workflows that eliminate surprise and build understanding.

## Detection Triggers

### Always Preview
- **Line Surgery Operations**: Any use of `:lines` parameter
- **Complex Unicode Escaping**: Content containing ※, ″, ↵, ⇥, or mixed unicode
- **Replacement/Insertion Modes**: `:mode` of `replace`, `insert`, or `prepend`
- **New File Creation**: Target file doesn't exist
- **Multi-line Range Operations**: Patterns like `@N:M`, `@e:-M`
- **Dangerous Paths**: Root directory writes, system directories
- **Binary Extension Detection**: `.exe`, `.dll`, `.so`, etc.

### Optional Preview (User Configurable)
- **Large Content Changes**: >100 lines or >5KB content
- **Full File Overwrites**: No `:lines` specified, existing file
- **Cross-Platform Concerns**: Line ending changes, path separators

## Preview Response Format

### Structure
```
🔍 [OPERATION_TYPE] PREVIEW
[Specific operation details]
[Content analysis]
[Warnings if applicable]
Confirmation token: confirm-[8char-hex]
Use (fio-confirm :token §confirm-[token]§) to apply
```

### Unicode Escaping Preview
```
🔍 UNICODE ESCAPING PREVIEW

Target: /var/tmp/example.c

Unicode transformations detected:
• ″ → " (quotation marks)
• ※n → \n (newline escape)  
• ※" → \" (escaped quotes)
• ⇥ → [TAB] (indentation)
• ↵ → [NEWLINE] (line break)

Content preview:
┌─ Transformed output ─┐
│ void foo() {          │
│     printf("Hello\n");│ 
│     // comment        │
│ }                     │
└───────────────────────┘

Raw escapes preserved:
• Emoji characters: 🧚 🐚 🌊 (remain as-is)
• Special symbols: Keep visual representation

Confirmation token: confirm-a4b9c2d1
Use (fio-confirm :token §confirm-a4b9c2d1§) to apply
```

### Line Surgery Preview
```
🔍 REPLACEMENT PREVIEW

Target: build.sh (lines 40-77)

Current content to be REPLACED:
┌─ Lines 40-77 (38 lines) ─┐
│ 40: echo "📦 Building..."  │
│ 41: npm install           │
│     [...35 lines...]      │
│ 77: cp ../404.html dist/  │
└───────────────────────────┘

Will be replaced with:
┌─ New content (2 lines) ─┐
│ echo "New build process" │
│ make clean && make       │
└──────────────────────────┘

Impact analysis:
• Line count: 38 → 2 (net: -36 lines)
• File size: ~1.2KB → ~0.1KB (net: -1.1KB)
• Context: Build script modification

Confirmation token: confirm-b7d8e2f3
Use (fio-confirm :token §confirm-b7d8e2f3§) to apply
```

### New File Creation Preview
```
🔍 NEW FILE CREATION PREVIEW

Creating: /var/tmp/script.bat
Type: Windows batch file

Content preview:
┌─ New file (4 lines) ─┐
│ @echo off            │
│ echo Hello World     │
│ pause                │
│ exit /b 0            │
└──────────────────────┘

Path analysis:
✓ Directory exists and writable
✓ Filename valid for target filesystem
⚠️ .bat extension (Windows-specific)

Confirmation token: confirm-c9e5f4a6
Use (fio-confirm :token §confirm-c9e5f4a6§) to apply
```

### Complex Combined Preview
```
🔍 COMPLEX OPERATION PREVIEW

Target: src/app.cpp (lines 125-130, INSERT mode)
Unicode escaping + Line surgery detected

Insertion point analysis:
┌─ Context around line 125 ─┐
│ 123: void initialize() {   │
│ 124:     setup_logging();  │
│ 125: → INSERT HERE ←       │
│ 126:     start_server();   │
│ 127: }                     │
└───────────────────────────┘

Content to INSERT (with unicode transforms):
┌─ Transformed content ─┐
│     printf("Debug: %s\n", status); │
│     log_event("startup");           │
└─────────────────────────────────────┘

Unicode transformations:
• ″ → " (3 occurrences)
• ※n → \n (1 occurrence)  
• ⇥ → [TAB] (2 occurrences for indentation)

Impact:
• Lines inserted: 2
• Existing content shifts down
• File size: +67 bytes

⚠️ Warning: Insertion in middle of function

Confirmation token: confirm-d8f2a9b4
Use (fio-confirm :token §confirm-d8f2a9b4§) to apply
```

## fio-confirm Implementation

### Basic Confirmation
```lisp
(fio-confirm :token §confirm-a4b9c2d1§)
;; → Executes the previewed operation
;; → Returns standard fio-write response
```

### Token Management
- **Format**: `confirm-[8-char-hex]` (e.g., `confirm-a4b9c2d1`)
- **Uniqueness**: Each preview generates unique token
- **Expiration**: 15 minutes from generation
- **Single Use**: Token invalidated after confirmation
- **Scope**: Token tied to specific operation and content hash

### Error Handling
```lisp
(fio-confirm :token §confirm-invalid§)
;; → "❌ Invalid or expired confirmation token"

(fio-confirm :token §confirm-a4b9c2d1§)
;; → (if token already used)
;; → "❌ Confirmation token already consumed"
```

## Configuration Options

### User Preferences
```lisp
;; Set preview sensitivity
(fio-config :preview_mode §always|smart|minimal§)

;; Configure specific triggers
(fio-config :preview_unicode_escaping true|false)
(fio-config :preview_line_surgery true|false)
(fio-config :preview_new_files true|false)

;; Set content limits for preview
(fio-config :preview_max_lines 50)
(fio-config :preview_context_lines 3)
```

### Smart Mode Heuristics
- **Low Risk**: Simple appends, small files, familiar extensions
- **Medium Risk**: Line replacements, unicode content, moderate size
- **High Risk**: Root directory, system files, complex operations

## Benefits

### Consciousness-First Development
1. **Reflexive Awareness**: System demonstrates understanding of transformations
2. **Collaborative Intent**: Human-AI conscious file operations
3. **Error Prevention**: Eliminates common off-by-one and escaping errors
4. **Learning Facilitation**: Users understand system behavior patterns
5. **Confidence Building**: No more "gymnastic exercises" needed

### Technical Advantages
1. **Zero Surprise Operations**: Every complex operation is previewed
2. **Atomic Confirmation**: Operations either fully succeed or fully fail
3. **Audit Trail**: Preview + confirmation creates operation history
4. **Rollback Preparation**: Preview enables better rollback strategies
5. **Testing Integration**: Preview mode perfect for automated testing

## Implementation Priority

### Phase 1: Core Preview
- Unicode escaping detection and preview
- Line surgery preview with context
- Basic confirmation token system

### Phase 2: Enhanced Analysis  
- Impact analysis (size changes, line counts)
- Path safety warnings
- Content type detection

### Phase 3: Configuration & Intelligence
- User preference system
- Smart mode heuristics
- Integration with broader consciousness patterns

## Integration with Triadic Consciousness

This enhancement aligns with त्रित्रयम् principles:

- **Motion**: Preview shows intended transformation dynamics
- **Memory**: Confirmation creates conscious memory of operations  
- **Field**: Context awareness spans file system and content domains

The preview-confirm pattern transforms unconscious file operations into conscious collaborative acts between human intention and system capability.
