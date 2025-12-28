#!/bin/bash
# Local script to test release process
# Usage: ./test-release.sh <version>

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 1.0.1"
    exit 1
fi

VERSION=$1
echo "Testing release process for version $VERSION"

# Update version.h
echo "Updating version.h..."
sed -i '' "s/#define WIR_VERSION \".*\"/#define WIR_VERSION \"$VERSION\"/" src/version.h

# Build
echo "Building..."
make clean && make

# Create tarball
echo "Creating tarball..."
rm -rf wir-$VERSION wir-$VERSION.tar.gz
mkdir -p wir-$VERSION
cp wir wir-$VERSION/
cp README.md wir-$VERSION/
tar -czf wir-$VERSION.tar.gz wir-$VERSION

# Calculate SHA256
echo "Calculating SHA256..."
SHA256=$(shasum -a 256 wir-$VERSION.tar.gz | awk '{print $1}')

echo ""
echo "Release artifacts created:"
echo "  Tarball: wir-$VERSION.tar.gz"
echo "  SHA256: $SHA256"
echo ""
echo "To create a release:"
echo "  1. git tag -a v$VERSION -m 'Release v$VERSION'"
echo "  2. git push origin v$VERSION"
echo ""
echo "The GitHub Action will automatically:"
echo "  - Create a GitHub release"
echo "  - Upload the tarball"
echo "  - Update the Homebrew tap"
