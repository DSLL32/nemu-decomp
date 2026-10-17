# Profile plugin

This reconstruction compiles to all **373,248 bytes** of
`master/Plugin/profile.dll`, including its original code, data, imports,
exports, relocations, form resource, package information and initialization
order. The final SHA-256 is
`b082cfd8a8146a5ea967c219e699658d9232347c66f8469a4a741e59becc5e68`.

From the repository root:

```sh
python3 scripts/build_profile.py --report analysis/profile-comparison.json
```

The default build creates a new directory and a dedicated 32-bit Wine prefix
under `~/.ai-tmp`. It recompiles every application unit, refreshes the DCUs with
`-M`, then links their cached interfaces to reproduce Delphi's original package
and initialization traversal. `--work ~/.ai-tmp/<scratch>/build` reuses a work
directory and prefix while still recompiling all units.

The compiler is the unchanged Delphi 6 General Update 1 Personal DCC32,
6.0.6.190, used with the unchanged **RTM** RTL/VCL compiled units. The
`toolchains/d6rtm` name describes the runtime selection. This reproducible
combination does not establish which compiler executable produced the original
DLL. File provenance, hashes and a fresh installer extraction command are in
`toolchains/profile-toolchain.json` and `scripts/install_profile_toolchain.py`.

The only post-link operation restores 34 resource-directory DOS timestamps to
`2001-09-24T21:04:20`. The unmodified compiler output is retained as
`profile.unstamped.dll`; no executable code, initialized data, resource payload,
package entry, initialization entry or relocation is patched. The build report
records every restored offset and the hash of the unstamped file. A content
addressed artifact directory preserves both DLLs, the MAP, build logs, source
and build-helper snapshots, and all toolchain hashes.

An artifact can be checked independently with:

```sh
python3 scripts/verify_profile.py /path/to/profile.dll
```

The verifier accepts only full-file equality and rejects a modified reference
DLL. Its section, resource, export and published-form diagnostics use literal
bytes and values; no address normalization is used for acceptance.

The original package and RTTI retain the seven source-unit names and
`TFormProfiler`/`TOptimizer`. The flat source directory is inferred because no
original path survives. `UnitProfiling.dfm` is a text conversion of the original
form resource; recompilation produces the identical binary stream. Private
field names, nonpublished method names and global identifiers are inferred.
In particular, `ProfilerFormHandle` and `DoProfiling` describe the recovered
roles and reproduce Delphi's imported-data ordering; those spellings are not
uniquely recoverable. `Profiler.pas` preserves the linked empty unit and its
dependency graph; any discarded original declarations remain unknown.

`Optimizer.pas` reconstructs the performance-counter timer and bar display.
`UnitProfiling.pas` restores the reset, enable, accumulate, timer and window
message handlers. `profile.dpr` restores all four Tool exports, including the
original event-to-process mapping. `UnitDecs.pas`, `RCP.pas` and `UnitMain.pas`
retain the host-interface layout, register-name tables and plugin globals.
