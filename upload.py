# tool to download builds from GitHub and generate shell commands to upload to conda-nion and pypi.

import json
import pathlib
import subprocess
import typing
import zipfile

owner = "nion-software"
repo = "nionui-tool"
directory = "upload"
channel = "nion"

def query(q: str, stdout=None) -> typing.Optional[typing.Dict]:
    stdout = stdout or subprocess.PIPE
    c = subprocess.run(["gh", "api", q], cwd=directory, stdout=stdout, stderr=subprocess.STDOUT)
    if c.returncode == 0 and c.stdout:
        o = c.stdout.decode('utf-8')
        if o:
            return json.loads(o)
    return None

def get_run_id() -> typing.Optional[str]:
    d = query(f"repos/{owner}/{repo}/actions/runs?page=1&per_page=1")
    if d:
        return d["workflow_runs"][0]["id"]
    return None

def get_artifacts(run_id: str) -> typing.List[dict]:
    d = query(f"repos/{owner}/{repo}/actions/runs/{run_id}/artifacts")
    if d:
        return d["artifacts"]
    return list()

pathlib.Path(directory).mkdir(exist_ok=True)

artifacts = get_artifacts(get_run_id())
artifacts_lookup = {a["name"]: a for a in artifacts}

upload_paths = dict()

for platform in ("linux", "macos", "win"):
    for py_version in ("3.8", "3.9"):
        for pkg in ("whl", "conda"):
            name = f"{repo}-{platform}-{pkg}-{py_version}"
            url = artifacts_lookup[name]["url"]
            artifact_id = artifacts_lookup[name]["id"]
            zip_filepath = pathlib.Path(directory + "/" + name + ".zip")
            zip_filepath.unlink(missing_ok=True)
            with open(zip_filepath, "w") as f:
                query(f"repos/{owner}/{repo}/actions/artifacts/{artifact_id}/zip", stdout=f)
            with zipfile.ZipFile(zip_filepath, 'r') as z:
                zip_filename = z.namelist()[0]
                zip_path = pathlib.Path(f"{directory}/{platform}/{zip_filename}")
                zip_path.unlink(missing_ok=True)
                z.extract(zip_filename, path=f"{directory}/{platform}")
                print(f"extracted {directory}/{platform}/{zip_filename} ({zip_path.stat().st_size})...")
                upload_paths.setdefault(pkg, list()).append(zip_path)
            zip_filepath.unlink(missing_ok=True)

print()

for upload_path in sorted(set(upload_paths["conda"])):
    print(f"anaconda upload --user {channel} {upload_path}")

print()

for upload_path in sorted(set(upload_paths["whl"])):
    print(f"twine upload {upload_path}")

print()

for upload_path in sorted(set(upload_paths["conda"] + upload_paths["whl"])):
    print(f"lftp -u xnion-swiftsite -e \"cd files/latest; put {upload_path}; quit\" sftp://nion.com")
