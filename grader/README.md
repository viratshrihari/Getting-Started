# Maintainer notes

The CLI and grader are C. No Python packages required.

`run NAME` compiles locally and compares against the README samples (case 01,
or cases 01–03 for Long Cow I).
It never commits, pushes, or marks an assignment complete.

`submit` commits only the chosen solution, then runs `git push --set-upstream origin HEAD`.
The subject is `Submit: LANGUAGE/ASSIGNMENT`. The push workflow grades only that assignment.
Unchanged solutions can be resubmitted. A failed push preserves the commit for retry.

`list` reads submission commits and GitHub Actions results through `gh`.
Progress is cached in ignored `.getting_started/`; fresh clones recover it from Git history.
Edits after a submission show `*`. Unavailable GitHub results keep the saved status.

Twenty fixed cases per assignment live in `testcases/`, shared across languages.
They include examples, seeded random data, edge cases, and N = 100000.
Student commands and CI never generate tests.

Limits: 2 seconds per test (Java: 4), 30 seconds to compile, 4 MiB output.
Numeric exercises compare whitespace-separated tokens. Moo Chant exercises preserve
spaces and line breaks; CRLF and one final output newline are allowed.
Children are killed on timeout.
Verdicts: AC, WA, TLE, RE, CE, OLE. GitHub's run log gives details.

The Dockerfile is shared by Codespaces and CI: GCC 16, Java 26, latest stable Python at image build.
C uses `-std=c2y`; C++ uses `-std=gnu++26`. Standard support depends on GCC's implementation.
Java uses `--release 26 --enable-preview`. Rebuild the container to update Python.
For local grading, install matching runtimes; `gcc-16`/`g++-16` are preferred when present.

Python uses `-I` for both compilation and execution, ignoring inherited `PYTHONPATH`,
`PYTHONHOME`, and user site packages. Compilation checks syntax without running
module-level code. Each case gets a fresh Python process and the entire input file
on stdin; module-level `input()` and globals work normally.
To choose an interpreter explicitly, set `GETTING_STARTED_PYTHON` to its executable
path, for example `GETTING_STARTED_PYTHON=/usr/bin/python3 getting_started -py run 01-echo`.
The selected interpreter is used for both syntax checking and execution.

New exercises: Moo Chant I (no input), Collatz, Moo Chant II (one line), Long Cow I,
Long Cow II, Moo Chant III (until EOF), and Moo Chant IV (repeat the multiline chant).
Existing assignment names remain unchanged. Moo Chant IV removes one final input
line ending before repetition; Long Cow II breaks length ties by input order.

```sh
make test                              # isolated, including a local Git remote
./getting_started --ci 'Submit: c/01-echo'  # grade locally without pushing
make fixtures                          # regenerate fixed tests + READMEs; preserve solutions
```

Add exercises in `catalog.h`, extend `tools/generate.c`, then run `make fixtures`.
Commit the resulting files. Language time limits live in `catalog.h`.

References: [GCC standards](https://gcc.gnu.org/onlinedocs/gcc/Standards.html),
[C++26 support](https://gcc.gnu.org/projects/cxx-status.html),
[GitHub run queries](https://cli.github.com/manual/gh_run_list).
