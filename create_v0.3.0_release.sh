#!/bin/bash
# LabDb v0.3.0 Release Tagging Script
# Creates official version tag for stable release

set -e  # Exit on any error

echo "🎺 LabDb v0.3.0 Release Tagging"
echo "==============================="

# Verify we're in the right repository
if [[ ! -f "CMakeLists.txt" ]] || [[ ! -d "include/LabDb" ]]; then
    echo "❌ Error: Must run from LabDb repository root"
    echo "   Current directory: $(pwd)"
    echo "   Expected: /Users/nporcino/dev/Lab/LabDb"
    exit 1
fi

echo "✅ Verified LabDb repository location"

# Check git status
if ! git diff-index --quiet HEAD --; then
    echo "⚠️  Warning: Working directory has uncommitted changes"
    echo "   Run 'git status' to see pending changes"
    echo "   Commit or stash changes before creating release tag"
    exit 1
fi

echo "✅ Working directory clean"

# Verify we're on main branch
current_branch=$(git branch --show-current)
if [[ "$current_branch" != "main" ]]; then
    echo "⚠️  Warning: Not on main branch (current: $current_branch)"
    echo "   Switch to main branch before creating release tag"
    echo "   Run: git checkout main"
    exit 1
fi

echo "✅ On main branch"

# Check if tag already exists
if git tag -l | grep -q "^v0.3.0$"; then
    echo "⚠️  Warning: Tag v0.3.0 already exists"
    echo "   Use 'git tag -d v0.3.0' to delete existing tag first"
    echo "   Or increment version number for new release"
    exit 1
fi

echo "✅ Tag v0.3.0 available for creation"

# Get current commit hash for reference
current_commit=$(git rev-parse HEAD)
echo "📋 Current commit: $current_commit"

# Create the release notes commit if needed
if [[ ! -f "RELEASE_NOTES_v0.3.0.md" ]]; then
    echo "❌ Error: Release notes file missing"
    echo "   Expected: RELEASE_NOTES_v0.3.0.md"
    exit 1
fi

# Stage and commit release notes if not already committed
if git status --porcelain | grep -q "RELEASE_NOTES_v0.3.0.md"; then
    echo "📝 Committing release notes..."
    git add RELEASE_NOTES_v0.3.0.md
    git commit -m "Add release notes for v0.3.0

🎯 LabDb v0.3.0: TID Architecture + FetchContent Ready

Complete release documentation covering:
- Revolutionary TID-based storage architecture (4.18× efficiency)
- Professional CMake FetchContent integration
- Comprehensive Python bindings with triadic consciousness
- Performance validation and benchmarking results
- Consumer documentation and integration guides
- Migration tools and compatibility information
- Known issues and future roadmap

Ready for professional consumption by LabEuclid and Lab ecosystem projects."
fi

echo "✅ Release notes committed"

# Show summary of what will be tagged
echo ""
echo "📋 Release Summary"
echo "=================="
echo "Version: v0.3.0"
echo "Codename: TID Architecture + FetchContent Ready"
echo "Commit: $current_commit"
echo "Branch: $current_branch"
echo ""

# Show recent commits that will be included
echo "📝 Recent commits to be included in v0.3.0:"
git log --oneline -10

echo ""
echo "🎯 Major Features in v0.3.0:"
echo "- Revolutionary TID-based storage (4.18× efficiency)"
echo "- Professional CMake FetchContent integration"
echo "- Complete Python bindings with triadic consciousness"
echo "- Comprehensive consumer documentation"
echo "- Integration validation and test suite"
echo "- Ready for LabEuclid consumption"

echo ""
read -p "🚀 Create v0.3.0 release tag? (y/N): " -n 1 -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "❌ Release tagging cancelled"
    exit 0
fi

# Create the annotated tag
echo "🏷️  Creating annotated tag v0.3.0..."

git tag -a v0.3.0 -m "LabDb v0.3.0: TID Architecture + FetchContent Ready

🚀 MAJOR RELEASE: Revolutionary storage efficiency + professional integration

REVOLUTIONARY STORAGE ARCHITECTURE:
- Complete TID-based architecture with 4.18× storage efficiency
- TermDictionary: String↔TermID mapping with LMDB persistence
- TIDSequenceGenerator: Unique triple ID allocation with atomic transactions
- TripleStore: Central storage with rich provenance metadata
- Crown indices refactored to store compact 8-byte TIDs

PROFESSIONAL DEPENDENCY MANAGEMENT:
- CMake FetchContent integration for naive consumers
- Modern CMake patterns with proper target exports
- Complete Python bindings with triadic consciousness API
- Comprehensive test suites and integration validation

PERFORMANCE & VALIDATION:
- 4.18× storage efficiency validated with migration tools
- Performance benchmarking: 552 inserts/sec, 672 queries/sec
- 71k triple validation with Euclid-inspired datasets
- Memory usage <50MB for complete datasets

CONSUMER READY:
- Complete consumer documentation (docs/consuming_labdb.md)
- Updated README with quick start and integration examples
- Professional troubleshooting guides and best practices
- Ready for LabEuclid integration and MCP server development

BREAKING CHANGES:
- Storage format requires migration from v0.2.x
- Python bindings use new pybind11 architecture
- TriadicQuery class completely redesigned

MIGRATION:
- Automatic migration tools with validation
- Backward compatibility for basic NonoStore operations
- Performance validation confirms efficiency improvements

NEXT PROJECTS:
✅ LabEuclid integration ready
✅ Professional dependency consumption proven
✅ Triadic consciousness accessible from C++ and Python
✅ Production handoff complete

For complete details see: RELEASE_NOTES_v0.3.0.md"

echo "✅ Tag v0.3.0 created successfully!"

# Verify tag was created
if git tag -l | grep -q "^v0.3.0$"; then
    echo "✅ Tag verification: v0.3.0 exists"
else
    echo "❌ Tag verification failed"
    exit 1
fi

# Show tag details
echo ""
echo "📋 Tag Details:"
git show v0.3.0 --no-patch --format="Tag: %D%nCommit: %H%nAuthor: %an <%ae>%nDate: %ad%nMessage: %s"

echo ""
echo "🎉 LabDb v0.3.0 Release Tagged Successfully!"
echo "==========================================="
echo ""
echo "✅ Next steps:"
echo "   1. Push tag to remote: git push origin v0.3.0"
echo "   2. Create GitHub release with RELEASE_NOTES_v0.3.0.md"
echo "   3. Update dependent projects to use v0.3.0"
echo "   4. Begin LabEuclid integration with stable release"
echo ""
echo "🚀 LabDb is now ready for professional consumption!"
echo "   Use in CMakeLists.txt: GIT_TAG v0.3.0"
echo ""
echo "📚 Documentation:"
echo "   - Consumer guide: docs/consuming_labdb.md"
echo "   - Release notes: RELEASE_NOTES_v0.3.0.md"
echo "   - Integration examples: README.md"

# Optional: Show how to use the new release
echo ""
echo "💡 Usage example for consumers:"
echo "cmake
include(FetchContent)
FetchContent_Declare(
    LabDb
    GIT_REPOSITORY https://github.com/meshula/LabDb9.git
    GIT_TAG        v0.3.0
)
FetchContent_MakeAvailable(LabDb)
target_link_libraries(myapp PRIVATE LabDb::LabDb)"

echo ""
echo "🎯 Professional triadic consciousness database ready for ecosystem integration!"
