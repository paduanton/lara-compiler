"""Export and validate a stage snapshot on Linux, including uncommitted sources."""
import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import stat
import subprocess
import tarfile
import tempfile
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
GENERATED = {"src/lex.yy.c", "src/parser.tab.c", "src/parser.tab.h"}
EXTENSIONS = {"src": {".c", ".h", ".l", ".y"},
              "tests": {".c", ".py", ".sh", ".lc", ".expected", ".md"},
              "tools": {".py", ".sh", ".md"}, "docker": {".yml", ".yaml"}}


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def snapshot():
    paths = [ROOT / name for name in ["Makefile", "README.md", "STAGE", ".gitignore"]]
    if (ROOT / ".gitattributes").is_file():
        paths.append(ROOT / ".gitattributes")
    for directory, extensions in EXTENSIONS.items():
        for path in (ROOT / directory).rglob("*"):
            relative = path.relative_to(ROOT)
            if path.is_symlink():
                raise ValueError(f"Cannot export a symbolic link: {relative}")
            if any(part.startswith(".") or part == "__pycache__" for part in relative.parts):
                continue
            if path.is_file() and (path.suffix in extensions or path.name == "Dockerfile"):
                if relative.as_posix() not in GENERATED:
                    paths.append(path)
    optimization = ROOT / "doc/otimizacao.md"
    if optimization.is_file():
        paths.append(optimization)
    return {path.relative_to(ROOT).as_posix(): path.read_bytes() for path in sorted(paths)}


def git_info(*args):
    env = {**os.environ, "GIT_OPTIONAL_LOCKS": "0"}
    result = subprocess.run(["git", "-c", f"safe.directory={ROOT}", *args], cwd=ROOT,
                            capture_output=True, text=True, env=env, timeout=30)
    if result.returncode:
        raise RuntimeError(result.stderr)
    return result.stdout.strip()


def write_archives(files, destination, stage):
    zip_path = destination / f"lara-etapa-{stage}-projeto.zip.pending"
    tar_path = destination / f"lara-etapa-{stage}-entrega.tar.gz.pending"
    with zipfile.ZipFile(zip_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, data in files.items():
            info = zipfile.ZipInfo(f"lara-etapa-{stage}/{name}")
            info.create_system = 3
            info.compress_type = zipfile.ZIP_DEFLATED
            mode = 0o755 if name.endswith(".sh") else 0o644
            info.external_attr = (stat.S_IFREG | mode) << 16
            archive.writestr(info, data)
    with tarfile.open(tar_path, "w:gz") as archive:
        for name, data in files.items():
            info = tarfile.TarInfo(name)
            info.mode = 0o755 if name.endswith(".sh") else 0o644
            info.size = len(data)
            archive.addfile(info, io.BytesIO(data))
    return zip_path, tar_path


def extract_checked(archive_path, folder, files, stage, kind):
    if kind == "zip":
        with zipfile.ZipFile(archive_path) as archive:
            prefix = f"lara-etapa-{stage}/"
            expected = {prefix + name for name in files}
            if set(archive.namelist()) != expected or len(archive.namelist()) != len(expected):
                raise ValueError("ZIP inventory differs from snapshot")
            contents = {name: archive.read(prefix + name) for name in files}
    else:
        with tarfile.open(archive_path) as archive:
            members = archive.getmembers()
            if {m.name for m in members} != set(files) or len(members) != len(files):
                raise ValueError("TAR inventory differs from snapshot")
            if not all(m.isfile() for m in members):
                raise ValueError("TAR contains a non-file entry")
            contents = {name: archive.extractfile(name).read() for name in files}
    for name, data in contents.items():
        target = (folder / name).resolve()
        if not target.is_relative_to(folder.resolve()) or data != files[name]:
            raise ValueError(f"Invalid archive entry: {name}")
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        target.chmod(0o755 if name.endswith(".sh") else 0o644)


def validate(archive_path, files, stage, kind, destination):
    commands = [["make", "-j4"], ["make", "test"],
                ["make", "ast-dot", "FILE=tests/valid/01_soma.lc"]]
    if kind == "zip":
        commands.append(["make", "test-memory"])
    env = os.environ.copy()
    for key in ["MAKEFLAGS", "MFLAGS", "MAKELEVEL"]:
        env.pop(key, None)
    with tempfile.TemporaryDirectory(prefix=f"lara-stage-{stage}-{kind}-") as temporary:
        folder = Path(temporary)
        extract_checked(archive_path, folder, files, stage, kind)
        with (destination / f"validation-{kind}.log").open("w", encoding="utf-8") as log:
            for command in commands:
                print(f"Validating {kind}: {' '.join(command)}", flush=True)
                result = subprocess.run(command, cwd=folder, env=env, capture_output=True,
                                        text=True, timeout=120)
                log.write(f"$ {' '.join(command)}\n{result.stdout}{result.stderr}\n")
                log.flush()
                if result.returncode:
                    raise RuntimeError(f"{kind} validation failed; see {log.name}")
            svg = folder / "ast.svg"
            if not svg.is_file() or not ET.parse(svg).getroot().tag.endswith("svg"):
                raise RuntimeError(f"{kind}: Graphviz did not produce a valid SVG")
    return [" ".join(command) for command in commands]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--stage", type=int, required=True)
    args = parser.parse_args()
    stage = int((ROOT / "STAGE").read_text().strip())
    if args.stage != stage:
        parser.error(f"This checkout contains stage {stage}, not stage {args.stage}")
    if platform.system() != "Linux":
        parser.error("Run inside the Linux development container: make package")
    files = snapshot()
    revision = git_info("rev-parse", "HEAD")
    working_tree = git_info("status", "--porcelain", "--untracked-files=all")
    now = datetime.now(timezone.utc)
    destination = ROOT / "docs/releases" / f"etapa-{stage}" / now.strftime("%Y%m%d-%H%M%S-%f")
    destination.mkdir(parents=True, exist_ok=False)
    zip_path, tar_path = write_archives(files, destination, stage)
    try:
        checks = {kind: validate(path, files, stage, kind, destination)
                  for kind, path in [("zip", zip_path), ("tar", tar_path)]}
        if snapshot() != files:
            raise RuntimeError("Project files changed during packaging; rebuild the snapshot")
        outputs = {}
        for pending in [zip_path, tar_path]:
            final = pending.with_suffix("")
            pending.rename(final)
            outputs[final.name] = sha256(final.read_bytes())
        inventory = {name: sha256(data) for name, data in files.items()}
        metadata = {"stage": stage, "status": "ready", "created_utc": now.isoformat(),
                    "source_revision": revision, "working_tree": working_tree,
                    "platform": platform.platform(), "artifacts": outputs,
                    "files": inventory, "validation": checks}
        (destination / "manifest.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        lines = [f"# Entrega da Etapa {stage}", "", "Estado: artefatos validados; envio pendente.", "",
                 f"Data UTC: {now.isoformat()}", f"Revisão de referência: `{revision}`", "",
                 "Snapshot dos arquivos atuais, incluindo alterações ainda não commitadas.", "",
                 "## Arquivos compactados", "", "| Arquivo | SHA-256 |", "|---|---|"]
        lines += [f"| {name} | `{digest}` |" for name, digest in outputs.items()]
        lines += ["", "## Verificação", "", "Os dois formatos foram extraídos separadamente,",
                  "compilados do zero e validados com make test e geração de AST/SVG.",
                  "O snapshot extraído do ZIP também passou em make test-memory (Valgrind).",
                  "Logs: validation-zip.log e validation-tar.log.", "",
                  "As limitações do estágio estão descritas no README incluído em ambos.", "",
                  "Identificação do grupo e prazo externo confirmado: preencher antes do envio.", "",
                  "## Estado do checkout", "", "```text", working_tree or "clean", "```", "",
                  "## Inventário do snapshot", "", "| Arquivo | SHA-256 |", "|---|---|"]
        lines += [f"| {name} | `{digest}` |" for name, digest in inventory.items()]
        (destination / "manifesto.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    except Exception as error:
        (destination / "FAILED.txt").write_text(str(error) + "\n", encoding="utf-8")
        raise
    print(f"Validated delivery: {destination}", flush=True)
    for name, digest in outputs.items():
        print(f"{name}: {digest}")


if __name__ == "__main__":
    main()
