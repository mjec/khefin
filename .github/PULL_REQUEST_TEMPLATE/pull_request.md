---
name: Pull request
about: Propose changes to this project
title: ''
labels: ''
assignees: ''

---

Use this checklist to prepare your pull request for merging. Pull requests that do not have all of these properties will need to be revised before they can be merged.

- [ ] Issue number referenced in commit message(s)

- [ ] Changes squashed to a single commit, unless logically distinct (use [`Co-authored-by`](https://help.github.com/en/github/committing-changes-to-your-project/creating-a-commit-with-multiple-authors) if necessary)

- [ ] No merge commits

- [ ] `APPVERSION` updated in `metadata.make`, in accordance with [semantic versioning](https://semver.org/)

- [ ] `make format` has been run to conform to code style

- [ ] `make lint` finishes with no non-suppressed warnings (please do not add new suppressions without flagging this)

- [ ] `make shellcheck` finishes with no non-suppressed warnings (please do not add new suppressions without flagging this)

- [ ] Checked that `make`, `make release`, `make install` and `make clean` all work as expected

- [ ] Checked that `make clean; make` compiles successfully with no warnings or errors

- [ ] `CONTRIBUTORS.md` and `CHANGELOG.md` are updated (if required)

- [ ] `README.md` is updated (if required)

- [ ] `src/help.c`, `docs/bash-completion.m4` and `docs/manpage.m4` are updated (if required)

- [ ] You accept that your contributions will be _irrevocably_ released under the license specified in `LICENSE`
