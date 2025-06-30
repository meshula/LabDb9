#!/usr/bin/env python3
"""
Remove TripleStore bindings from Python bindings to fix linking issues
"""

import re

# Read the file
with open('/Users/nporcino/dev/Lab/LabDb/python/bindings.cpp', 'r') as f:
    content = f.read()

# Find the start of TripleStore class definition
start_pattern = r'py::class_<LabDb::TripleStore>'
start_match = re.search(start_pattern, content)

if start_match:
    start_pos = start_match.start()
    
    # Find the matching closing bracket for this class
    # We need to count parentheses/brackets to find the end
    bracket_count = 0
    i = start_pos
    while i < len(content):
        if content[i] == '(':
            bracket_count += 1
        elif content[i] == ')':
            bracket_count -= 1
            if bracket_count == 0:
                # Found the end, but look for the trailing semicolon
                j = i + 1
                while j < len(content) and content[j] in ' \t\n':
                    j += 1
                if j < len(content) and content[j] == ';':
                    end_pos = j + 1
                    break
                else:
                    end_pos = i + 1
                    break
        i += 1
    
    # Also remove any related structures that follow
    remaining = content[end_pos:]
    
    # Remove TripleData, StringTriple, TripleStoreStats, and TripleFlags
    patterns_to_remove = [
        r'py::class_<LabDb::TripleStore::TripleData>.*?;',
        r'py::class_<LabDb::TripleStore::StringTriple>.*?;',
        r'py::class_<LabDb::TripleStore::Stats>.*?;',
        r'py::enum_<LabDb::TripleStore::TripleFlags>.*?\.export_values\(\);'
    ]
    
    for pattern in patterns_to_remove:
        remaining = re.sub(pattern, '', remaining, flags=re.DOTALL)
    
    # Reconstruct the file
    new_content = content[:start_pos] + remaining
    
    # Write the cleaned file
    with open('/Users/nporcino/dev/Lab/LabDb/python/bindings.cpp', 'w') as f:
        f.write(new_content)
    
    print("✅ Removed TripleStore bindings from Python bindings")
    print(f"Removed {len(content) - len(new_content)} characters")
else:
    print("❌ TripleStore class not found")
