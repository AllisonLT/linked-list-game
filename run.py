from pathlib import Path
import os
import subprocess
import sys


project_root = Path(__file__).resolve().parent
build_directory = project_root / "build"
executable_name = "LinkedListGame.exe" if os.name == "nt" else "LinkedListGame"
executable_path = project_root / executable_name


def run(command: list[str], *, cwd: Path | None = None) -> None:
    subprocess.run(command, check=True, cwd=cwd)


try:
    run(["cmake", "-S", str(project_root), "-B", str(build_directory)])
    run(["cmake", "--build", str(build_directory), "--target", "LinkedListGame", "--config", "Debug"])
    result = subprocess.run([str(executable_path)], cwd=project_root)
except subprocess.CalledProcessError as error:
    sys.exit(error.returncode)

sys.exit(result.returncode)
