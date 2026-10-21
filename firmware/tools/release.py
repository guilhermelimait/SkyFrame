"""Builds and signs a firmware release for GitHub.

    %USERPROFILE%\\.platformio\\penv\\Scripts\\python.exe tools\\release.py

1. Raise FW_VERSION in src/app.h first (the display only installs a NEWER version).
2. This builds the firmware, signs it with your private key and writes
   release/firmware.bin and release/firmware.sig.
3. On GitHub: Releases → "Draft a new release" → tag  v<version>  → attach both files → Publish.
"""
import hashlib, os, re, shutil, subprocess, sys
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import ec

root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(root)
version = re.search(r'#define FW_VERSION "([^"]+)"', open("src/app.h", encoding="utf-8").read()).group(1)
# The private key lives OUTSIDE the repository so it can never be uploaded by accident:
#   <folder containing skyframe>/skyframe-keys/ota_signing_key.pem
# or wherever the SKYFRAME_SIGNING_KEY environment variable points.
key_path = os.environ.get("SKYFRAME_SIGNING_KEY") or os.path.join(os.path.dirname(os.path.dirname(root)), "skyframe-keys", "ota_signing_key.pem")
if not os.path.exists(key_path):
    sys.exit(f"Signing key not found at {key_path}. Without the original key the display refuses updates.")
key = serialization.load_pem_private_key(open(key_path, "rb").read(), password=None)

# the public key built into the firmware must match this private key
pub_pem = key.public_key().public_bytes(serialization.Encoding.PEM, serialization.PublicFormat.SubjectPublicKeyInfo).decode()
hdr = open("src/ota_pubkey.h", encoding="utf-8").read()
if pub_pem.strip().splitlines()[1] not in hdr:
    sys.exit("src/ota_pubkey.h doesn't match the signing key: run tools/keygen.py first")

if "--no-build" not in sys.argv:
    pio = os.path.join(os.path.dirname(sys.executable), "pio")
    subprocess.run([pio if os.path.exists(pio + ".exe") else "pio", "run"], check=True)

os.makedirs("release", exist_ok=True)
shutil.copyfile(os.path.join(".pio", "build", "desk", "firmware.bin"), os.path.join("release", "firmware.bin"))
data = open(os.path.join("release", "firmware.bin"), "rb").read()
sig = key.sign(data, ec.ECDSA(hashes.SHA256()))          # DER signature of SHA-256(firmware)
key.public_key().verify(sig, data, ec.ECDSA(hashes.SHA256()))   # double-check before publishing
open(os.path.join("release", "firmware.sig"), "w").write(sig.hex())

print(f"""
Release v{version} is ready:
  release/firmware.bin   {len(data):,} bytes   sha256 {hashlib.sha256(data).hexdigest()[:16]}…
  release/firmware.sig   signature (verified)

On GitHub: Releases → Draft a new release → tag  v{version}  → attach both files → Publish.
The display picks it up at night, or right away with "Check for updates now" in its settings.""")
