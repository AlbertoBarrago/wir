# Release Process

This project uses GitHub Actions to automate releases and Homebrew tap updates.

## Prerequisites

Before the first release, you need to set up a GitHub secret:

1. **Create a Personal Access Token (PAT)** for your tap repository:
   - Go to GitHub Settings → Developer settings → Personal access tokens → Tokens (classic)
   - Click "Generate new token (classic)"
   - Give it a name like "Homebrew Tap Update"
   - Select scopes: `repo` (Full control of private repositories)
   - Click "Generate token" and copy it

2. **Add the token to this repository**:
   - Go to your Wir repository Settings → Secrets and variables → Actions
   - Click "New repository secret"
   - Name: `HOMEBREW_TAP_TOKEN`
   - Value: Paste the token you created
   - Click "Add secret"

## How to Release

### Automatic Release (Recommended)

1. Update the version if needed (or let the workflow do it):
   ```bash
   # Optional: manually update version in src/version.h
   vim src/version.h
   ```

2. Commit your changes:
   ```bash
   git add .
   git commit -m "Prepare for release v1.0.1"
   git push
   ```

3. Create and push a tag:
   ```bash
   git tag -a v1.0.1 -m "Release v1.0.1"
   git push origin v1.0.1
   ```

4. The GitHub Action will automatically:
   - Update `src/version.h` with the tag version
   - Build the binary
   - Create a release tarball
   - Create a GitHub release
   - Update your Homebrew tap with the new version and SHA256

### Test Before Release

You can test the release process locally:

```bash
./.github/workflows/test-release.sh 1.0.1
```

This will:
- Update version.h
- Build the binary
- Create the tarball
- Calculate the SHA256
- Show you what will happen when you push the tag

## What Happens Automatically

When you push a tag matching `v*.*.*`:

1. **Version Update**: The tag version is extracted and updates `src/version.h`
2. **Build**: The project is built with `make clean && make`
3. **Packaging**: A tarball is created with the binary and README
4. **GitHub Release**: A new release is created with:
   - Auto-generated release notes
   - The tarball as a downloadable asset
5. **Homebrew Update**: Your `homebrew-tap` repository is updated:
   - Formula is updated with new version, URL, and SHA256
   - Changes are committed and pushed automatically

## Versioning

This project uses [Semantic Versioning](https://semver.org/):
- **MAJOR.MINOR.PATCH** (e.g., 1.0.0)
- **MAJOR**: Breaking changes
- **MINOR**: New features (backwards compatible)
- **PATCH**: Bug fixes (backwards compatible)

## Troubleshooting

### Action fails with "permission denied" on tap update

- Make sure you've created the `HOMEBREW_TAP_TOKEN` secret
- Ensure the token has `repo` scope
- Verify the token hasn't expired

### Version mismatch

- The workflow extracts version from the git tag
- The tag must match the pattern `v*.*.*` (e.g., `v1.0.1`)
- The workflow will update `src/version.h` automatically

### Build fails

- Check the GitHub Actions logs
- Test locally with `make clean && make`
- Ensure all dependencies are available

## Manual Homebrew Update

If you need to update the tap manually:

```bash
cd /path/to/homebrew-tap
vim Formula/wir.rb  # Update version, url, and sha256
git add Formula/wir.rb
git commit -m "Update wir to <version>"
git push
```
