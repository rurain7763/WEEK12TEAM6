import sys
import subprocess
import uuid
import shutil
from pathlib import Path

if len(sys.argv) < 2:
    print("Usage: python IndexSources.py <pdb-path>")
    sys.exit(1)

script_path = Path(__file__).resolve()
root_path = script_path.parent.parent 
source_path = root_path / "Source"
source_store_path = Path(r"\\DESKTOP-45SMRRK\SymbolServer\Sources")
pdb_path = sys.argv[1]

try:
    result = subprocess.run(
        ["srctool", "-r", pdb_path],
        capture_output=True,
        text=True,
    )
except FileNotFoundError:
    print("The required file was not found.")
    sys.exit(1)

identifier = uuid.uuid4().hex
entries = []

for line in result.stdout.splitlines():
    line = line.strip()

    # 빈 문자나 주석은 관심없음
    if not line:
        continue

    origin_path = Path(line).resolve()

    # 파일이 아니면 관심 없음
    if not origin_path.is_file():
        continue

    # 우리 엔진과 관련된 파일만 관심이 있음
    if not origin_path.is_relative_to(source_path):
        continue

    relative_path = origin_path.relative_to(source_path)
    final_path = source_store_path / identifier / relative_path

    entries.append(f"{origin_path}*{final_path}")

    final_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(origin_path, final_path)

ini = []
ini.append("SRCSRV: ini ------------------------------------------------")
ini.append("VERSION=1")
ini.append("SRCSRV: variables ------------------------------------------")
ini.append('SRCSRVTRG=%var2%')
ini.append('SRCSRVCMD=')
ini.append("SRCSRV: source files ---------------------------------------")
ini.extend(entries)
ini.append("SRCSRV: end ------------------------------------------------")

ini_path = source_store_path / identifier / "srcsrv.ini"
ini_path.write_text("\n".join(ini), encoding="utf-8")

result = subprocess.run(
    ["pdbstr", "-w", rf"-p:{pdb_path}", rf"-i:{ini_path}", "-s:srcsrv"],
    capture_output=True,
    text=True,
    check=True
)
