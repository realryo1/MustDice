# -*- coding: utf-8 -*-
"""
gamebuild.yml 相当のローカル Release ビルド。

- MustDice.sln を Release | x64 で Rebuild（Clean と Build を一度に実行）
- ルートの localbuild を先に空にしてからビルドし、成果物をコピー（option.yml は削除・上書きしない）
- zip 作成・version 更新はしない

使い方:
  python tool/local_release_build.py
"""

from __future__ import annotations

import glob
import os
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SLN = ROOT / "MustDice.sln"
OUT_DIR = ROOT / "localbuild"
RELEASE_BIN = ROOT / "x64" / "Release"
KEEP_NAMES = {"option.yml"}


def find_msbuild() -> Path:
    vswhere = (
        Path(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"))
        / "Microsoft Visual Studio"
        / "Installer"
        / "vswhere.exe"
    )
    if vswhere.is_file():
        r = subprocess.run(
            [
                str(vswhere),
                "-latest",
                "-requires",
                "Microsoft.Component.MSBuild",
                "-property",
                "installationPath",
            ],
            capture_output=True,
            text=True,
            check=False,
        )
        vs_path = (r.stdout or "").strip()
        if vs_path:
            for rel in (
                Path("MSBuild") / "Current" / "Bin" / "MSBuild.exe",
                Path("MSBuild") / "15.0" / "Bin" / "MSBuild.exe",
            ):
                p = Path(vs_path) / rel
                if p.is_file():
                    return p

    fallbacks = [
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe"),
        Path(r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"),
        Path(r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe"),
    ]
    for p in fallbacks:
        if p.is_file():
            return p
    raise FileNotFoundError("MSBuild.exe が見つかりません。Visual Studio を確認してください。")


def run_msbuild(msbuild: Path, extra: list[str]) -> None:
    cmd = [
        str(msbuild),
        str(SLN),
        "/p:Configuration=Release",
        "/p:Platform=x64",
        *extra,
    ]
    print(" ".join(cmd))
    r = subprocess.run(cmd, cwd=str(ROOT))
    if r.returncode != 0:
        raise SystemExit(r.returncode)


def wipe_out_dir() -> None:
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    for child in OUT_DIR.iterdir():
        if child.name in KEEP_NAMES:
            continue
        if child.is_dir():
            shutil.rmtree(child)
        else:
            child.unlink()


def copy_glob(pattern: str, dest: Path) -> None:
    dest.mkdir(parents=True, exist_ok=True)
    for src in glob.glob(pattern, root_dir=str(ROOT)):
        src_path = ROOT / src
        if src_path.is_file():
            dst = dest / src_path.name
            if dst.name in KEEP_NAMES and dst.exists():
                continue
            shutil.copy2(src_path, dst)


def collect_files() -> None:
    copy_glob("x64/Release/*.exe", OUT_DIR)

    shader_dest = OUT_DIR / "shader"
    shader_dest.mkdir(parents=True, exist_ok=True)
    copy_glob("shader/*.cso", shader_dest)

    readme = ROOT / "readme.md"
    if readme.is_file():
        shutil.copy2(readme, OUT_DIR / "readme.md")

    docs = ROOT / "document_ingame"
    if docs.is_dir():
        shutil.copytree(docs, OUT_DIR / "document_ingame")

    copy_glob("x64/Release/*.dll", OUT_DIR)

    assimp = ROOT / "assimp-vc143-mt.dll"
    if assimp.is_file():
        shutil.copy2(assimp, OUT_DIR / assimp.name)

    asset = ROOT / "asset"
    if asset.is_dir():
        shutil.copytree(asset, OUT_DIR / "asset")


def main() -> int:
    if not SLN.is_file():
        print(f"solution not found: {SLN}", file=sys.stderr)
        return 1

    msbuild = find_msbuild()
    print(f"Using MSBuild: {msbuild}")

    print(f"Wiping {OUT_DIR}...")
    wipe_out_dir()

    print("Rebuilding Release x64...")
    run_msbuild(msbuild, ["/t:Rebuild", "/m", "/v:minimal"])

    print("Collecting files...")
    collect_files()
    print(f"Done: {OUT_DIR}")
    return 0


if __name__ == "__main__":
    code = 1
    try:
        code = main()
    except FileNotFoundError as e:
        print(str(e), file=sys.stderr)
        code = 1
    except SystemExit as e:
        code = e.code if isinstance(e.code, int) else 1
    finally:
        try:
            input("Enter で閉じます...")
        except EOFError:
            pass
    raise SystemExit(code)
