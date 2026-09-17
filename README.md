# Getting Started

Fork this repo. Open your fork in Codespaces. Setup is automatic.
If GitHub asks, enable Actions in your fork.

```sh
getting_started -py start next
getting_started -py run 00-moo-chant-i
getting_started -py submit 00-moo-chant-i
getting_started -py start next
getting_started -py run 01-echo
getting_started -py submit 01-echo
getting_started -py list
```

Works from any folder. Edit the file shown by `start`.
`submit` commits that solution and pushes your current branch.
GitHub grades it; `list` refreshes the result.
`run NAME` compiles and checks the sample locally, showing expected and actual output.

GNU C++26 · C2y (newest C draft) · Java 26 (preview enabled) · latest stable Python.

Languages: `-py`, `-p`, `-python`, `-c`, `-cpp`, `-cc`, `-c++`, `-java`.
Commands: `list`, `start NAME`, `start next`, `run NAME`, `submit NAME`.
Command names also accept a leading `-`. No command means `list`.

`✓` completed · `.` not started · `*` started / grading / edited · `x` incorrect.

Local macOS/Linux (Windows: WSL): install a C compiler, Make, Git, and
[GitHub CLI](https://cli.github.com/). Then:

```sh
make install
gh auth login
```

Open a new terminal. Python/Java/C++ are needed locally only to run those solutions.

To update an existing fork: **Sync fork → Update branch** on GitHub, then
`git pull && make install` in your terminal. Commit your work before pulling.

[Maintainer notes](grader/README.md)
