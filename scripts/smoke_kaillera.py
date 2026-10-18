#!/usr/bin/env python3
"""Compare basic public API behavior of an original and rebuilt Kaillera DLL."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = '#include <windows.h>\n#include <stdio.h>\n#include <string.h>\ntypedef struct { char *app,*games;void *game,*chat,*drop,*more; } Infos;\nint main(int argc,char **argv) {\n HMODULE m; FARPROC init,shutdown; void (WINAPI *version)(char *);void (WINAPI *setinfos)(Infos *);int (WINAPI *modify)(void *,int);void (WINAPI *chat)(char *);\n char v[64]={0};char games[]="Example Game\\0Second Game\\0";Infos infos;int values=0x12345678,result;\n if(argc!=2)return 2;m=LoadLibrary(argv[1]);if(!m){printf("load error %lu\\n",GetLastError());return 3;}\n version=(void (WINAPI *)(char *))GetProcAddress(m,"_kailleraGetVersion@4");\n init=GetProcAddress(m,"_kailleraInit@0");shutdown=GetProcAddress(m,"_kailleraShutdown@0");\n setinfos=(void (WINAPI *)(Infos *))GetProcAddress(m,"_kailleraSetInfos@4");modify=(int (WINAPI *)(void *,int))GetProcAddress(m,"_kailleraModifyPlayValues@8");chat=(void (WINAPI *)(char *))GetProcAddress(m,"_kailleraChatSend@4");\n if(!version||!init||!shutdown||!setinfos||!modify||!chat)return 4;\n version(v);printf("version=%s\\n",v);if(strcmp(v,"0.84"))return 5;\n init();infos.app="Smoke Test 1.0";infos.games=games;infos.game=infos.chat=infos.drop=infos.more=0;setinfos(&infos);setinfos(&infos);result=modify(&values,4);printf("idle_modify=%d value=%08x\\n",result,values);chat("unused");shutdown();\n FreeLibrary(m);return result==-1&&values==0x12345678?0:6;\n}\n'

def winpath(path):
    return 'Z:' + str(Path(path).resolve()).replace('/', '\\')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    args = parser.parse_args()
    parent = Path.home() / '.ai-tmp'
    parent.mkdir(exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix='kaillera-smoke.', dir=parent))
    source = work / 'smoke.c'
    source.write_text(SOURCE)
    env = dict(os.environ, WINEPREFIX=str(work / 'wine'), WINEARCH='win32', WINEDEBUG='-all')
    def run(label, command):
        log = work / (label + '.log')
        with log.open('w') as stream:
            result = subprocess.run(command, cwd=work, env=env, stdout=stream,
                                    stderr=subprocess.STDOUT, timeout=120)
        if result.returncode:
            raise RuntimeError(f'{label}: exit {result.returncode}; {log.read_text(errors="replace")}')
        return log.read_text(errors='replace')
    run('build', [sys.executable, str(ROOT / 'scripts/run_toolchain.py'), 'vc6rtm', 'cl',
        '/nologo', '/MD', '/Fe' + winpath(work / 'smoke.exe'),
        '/Fo' + winpath(work / 'smoke.obj'), winpath(source), '/link', 'user32.lib'])
    outputs = []
    for label, dll in [('original', ROOT / 'master/kailleraclient.dll'), ('rebuilt', args.dll.resolve())]:
        output = run(label, ['xvfb-run', '-a', 'wine', winpath(work / 'smoke.exe'), winpath(dll)])
        outputs.append([line for line in output.splitlines() if line.startswith(('version=', 'idle_modify='))])
    if outputs[0] != outputs[1]:
        raise RuntimeError(f'API outputs differ: {outputs}')
    print('Differential API smoke passed: version, initialization, repeated infos replacement, idle input, chat, shutdown')
    print(f'Logs and Wine prefix: {work}')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
