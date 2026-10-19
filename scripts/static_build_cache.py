"""Content-checked cache and bounded tool execution for the static user build."""
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import threading
import time


def write_changed(path, data):
    path = Path(path)
    if isinstance(data, str):
        data = data.encode('utf-8')
    if not path.exists() or path.read_bytes() != data:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)


def fingerprint(paths):
    digest = hashlib.sha256()
    for path in sorted(set(map(Path, paths))):
        digest.update(str(path).encode() + b'\0')
        if path.is_file():
            data = path.read_bytes()
            digest.update(str(len(data)).encode() + b'\0' + data)
        else:
            digest.update(b'missing\0')
    return digest.hexdigest()


def files(*paths):
    return [p for root in map(Path, paths)
            for p in (sorted(root.rglob('*')) if root.is_dir() else [root]) if p.is_file()]


def worker_count():
    override = os.environ.get('STATIC_JOBS')
    if override is not None:
        count = int(override)
        if count < 1:
            raise ValueError('STATIC_JOBS must be positive')
        return count
    flags = os.environ.get('MAKEFLAGS', '')
    match = re.search(r'(?:^|\s)(?:-j\s*|--jobs[= ])(\d+)(?:\s|$)', flags)
    if match:
        return int(match[1])
    # GNU make removes -j1 from MAKEFLAGS; jobserver-free invocations remain
    # serial. Running this script directly or with bare `make -j` is bounded.
    if 'MAKELEVEL' in os.environ and not re.search(r'(?:^|\s)-j(?:\s|$)', flags):
        return 1
    return min(8, os.cpu_count() or 1)


class Cache:
    def __init__(self, directory, recipe):
        self.directory = Path(directory)
        self.directory.mkdir(exist_ok=True)
        self.recipe = recipe

    def stamp(self, key):
        return self.directory / (hashlib.sha256(key.encode()).hexdigest() + '.json')

    def load(self, key, signature):
        try:
            item = json.loads(self.stamp(key).read_text())
            if item['signature'] == [self.recipe, signature]:
                outputs = item['outputs']
                if outputs and all(Path(p).is_file() for p in outputs):
                    if fingerprint(outputs) == item['output_hash']:
                        return item['result']
        except (OSError, ValueError, KeyError, TypeError):
            pass
        return None

    def invalidate(self, key):
        self.stamp(key).unlink(missing_ok=True)

    def save(self, key, signature, outputs, result=True):
        outputs = list(map(str, outputs))
        item = dict(signature=[self.recipe, signature], outputs=outputs,
                    output_hash=fingerprint(outputs), result=result)
        write_changed(self.stamp(key), json.dumps(item))


class Tools:
    def __init__(self, root, work, env, cache, jobs):
        self.root, self.work, self.env, self.cache = root, work, env, cache
        self.jobs = jobs
        scratch_root = Path.home() / '.ai-tmp'
        scratch_root.mkdir(exist_ok=True)
        self.scratch = Path(tempfile.mkdtemp(prefix='nemu-static-tools.', dir=scratch_root))
        self.log = (work / 'build.log').open('w')
        self.lock = threading.Lock()
        self.started = time.monotonic()
        self.active = self.maximum = self.compiled = self.reused = 0
        self.pool = ThreadPoolExecutor(max_workers=jobs)
        self.pending = []

    def message(self, text):
        with self.lock:
            self.log.write(text + '\n')
            self.log.flush()

    def run(self, tool, args, cwd=None, chain='vc7'):
        command = [os.sys.executable, str(self.root / 'scripts/run_toolchain.py'),
                   chain, tool, *map(str, args)]
        with self.lock:
            self.active += 1
            self.maximum = max(self.maximum, self.active)
            started = time.monotonic()
            self.log.write(f'[{started - self.started:.3f}s start, {self.active} active] '
                           + ' '.join(command) + '\n')
            self.log.flush()
        # A private on-disk log avoids both interleaved compiler diagnostics
        # and Wine service processes keeping a subprocess PIPE open.
        with tempfile.TemporaryFile(dir=self.scratch) as output:
            try:
                result = subprocess.run(command, env=self.env, cwd=cwd or self.root,
                                        stdout=output, stderr=subprocess.STDOUT)
                output.seek(0)
                diagnostics = output.read().decode('utf-8', errors='replace')
            finally:
                with self.lock:
                    self.active -= 1
            self.message(f'[{time.monotonic() - self.started:.3f}s finish, '
                         f'{time.monotonic() - started:.3f}s elapsed] {tool}\n' + diagnostics)
        if result.returncode:
            raise RuntimeError(f'{tool} failed ({result.returncode}):\n{diagnostics}\n'
                               'See obj/static/build.log')

    def cached(self, tool, args, output, dependency_hash, cwd=None, chain='vc7'):
        signature = [tool, list(map(str, args)), str(cwd or self.root), chain, dependency_hash]
        key = str(output)
        if self.cache.load(key, signature) is not None:
            with self.lock:
                self.reused += 1
            return
        self.cache.invalidate(key)
        self.run(tool, args, cwd, chain)
        self.cache.save(key, signature, [output])
        with self.lock:
            self.compiled += 1

    def submit(self, *args, **kwargs):
        self.pending.append(self.pool.submit(self.cached, *args, **kwargs))

    def wait(self):
        pending, self.pending = self.pending, []
        # Consume every result so failures cannot leave background mutations
        # running after the caller starts linking or handles an exception.
        error = None
        for future in pending:
            try:
                future.result()
            except Exception as caught:
                if error is None:
                    error = caught
        if error:
            raise error

    def close(self):
        self.pool.shutdown(wait=True)
        self.log.close()
