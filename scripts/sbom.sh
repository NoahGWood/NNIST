#!/usr/bin/env bash
set -euo pipefail

REPO_NAME="${REPO_NAME:-$(basename "$(pwd)")}"
OUT_DIR="${OUT_DIR:-artifacts/sbom}"
FORMAT="${FORMAT:-cyclonedx-json}"   # cyclonedx-json | spdx-json | cyclonedx-xml | spdx-tag-value
SBOM_FILE="${SBOM_FILE:-$OUT_DIR/${REPO_NAME}.sbom.json}"
VULN_FILE="${VULN_FILE:-$OUT_DIR/${REPO_NAME}.vuln.json}"

mkdir -p "$OUT_DIR"

need() { command -v "$1" >/dev/null 2>&1; }

echo "[*] Checking tools..."
missing=0
for t in syft trivy; do
  if ! need "$t"; then
    echo "  - Missing: $t"
    missing=1
  fi
done
if [[ "$missing" -ne 0 ]]; then
  echo ""
  echo "Install examples:"
  echo "  syft:  https://github.com/anchore/syft#installation"
  echo "  trivy: https://github.com/aquasecurity/trivy#installation"
  exit 2
fi

echo "[*] Generating SBOM with syft -> $SBOM_FILE"
syft . -o "$FORMAT" > "$SBOM_FILE"

# Optional: validate CycloneDX SBOM if cyclonedx-cli is installed
if need cyclonedx; then
  echo "[*] Validating SBOM (cyclonedx-cli)"
  cyclonedx validate --input-file "$SBOM_FILE" >/dev/null
else
  echo "[i] cyclonedx-cli not found; skipping validation."
fi

echo "[*] Running vulnerability scan with trivy -> $VULN_FILE"
# Trivy can also output an SBOM, but here we use it for vulns against your repo content
trivy fs --scanners vuln,secret,misconfig --format json --output "$VULN_FILE" .

# Optional: create a simple manifest for audits
MANIFEST="$OUT_DIR/manifest.txt"
echo "[*] Writing manifest -> $MANIFEST"
{
  echo "repo: $REPO_NAME"
  echo "generated_utc: $(date -u +"%Y-%m-%dT%H:%M:%SZ")"
  echo "sbom: $(basename "$SBOM_FILE")"
  echo "vuln_report: $(basename "$VULN_FILE")"
  echo "sha256_sbom: $(sha256sum "$SBOM_FILE" | awk '{print $1}')"
  echo "sha256_vuln: $(sha256sum "$VULN_FILE" | awk '{print $1}')"
} > "$MANIFEST"

# Optional: sign artifacts if cosign is installed (keyless requires OIDC in CI)
if need cosign; then
  echo "[*] cosign found; signing artifacts (keyless if configured)..."
  cosign sign-blob --yes "$SBOM_FILE"  > "$SBOM_FILE.sig" 2>/dev/null || true
  cosign sign-blob --yes "$VULN_FILE"  > "$VULN_FILE.sig" 2>/dev/null || true
  cosign sign-blob --yes "$MANIFEST"   > "$MANIFEST.sig"  2>/dev/null || true
else
  echo "[i] cosign not found; skipping signing."
fi

echo "[✓] Done. Outputs in: $OUT_DIR"
