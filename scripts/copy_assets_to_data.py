Import("env")
from pathlib import Path

project_root = Path(env['PROJECT_DIR'])
source_dir = project_root / 'assets'
data_dir = project_root / 'data' / 'assets'

data_dir.mkdir(parents=True, exist_ok=True)

if source_dir.exists():
    for entry in list(data_dir.glob('*')):
        if entry.is_file():
            entry.unlink()
        elif entry.is_dir():
            for child in list(entry.rglob('*')):
                if child.is_file():
                    child.unlink()
            for child in sorted(entry.rglob('*'), reverse=True):
                if child.is_dir():
                    child.rmdir()
            entry.rmdir()

    for item in sorted(source_dir.iterdir()):
        if item.is_file():
            target = data_dir / item.name
            target.write_bytes(item.read_bytes())
