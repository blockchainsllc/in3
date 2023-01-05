## Merge request checklist

Check if your MR fulfills the following requirements:

- [ ] Tests for the changes have been added (for bug fixes / features)
- [ ] Docs have been reviewed and added / updated if needed (for bug fixes / features)
- [ ] Build was run locally
- [ ] Gitlab CI pipeline is green
- [ ] Lint has passed locally and any fixes were made for failures
- [ ] Format check clang-format, (ran [format_all.sh](https://git.slock.it/in3/in3-core/-/blob/develop/scripts/format_all.sh))
- [ ] Check for private keys or login info and remove them
- [ ] An example has been added or an existing one has been updated/reviewed if needed (bug fixes / new features / signature changes)
- [ ] Ran all the examples in your local machine. (ci doesn't run them)
- [ ] CHANGELOG.md has been updated. Add one if it does not have it. Add a line per MR
- [ ] Code coverage badge was updated. Add one if it does not have it.


## Merge request type

Do not submit updates to dependencies unless it fixes an issue. also try to limit your pull request to one type, submit multiple pull requests if needed. 

Check the type of change your MR introduces:
- [ ] Bugfix
- [ ] Feature
- [ ] Code style update (formatting, renaming)
- [ ] Refactoring (no functional changes, no api changes)
- [ ] Build related changes
- [ ] Documentation content changes


## What is the current behavior?
Describe the current behavior that you are modifying, or link to a relevant issue.

Issue Number: N/A


## What is the new behavior?
Describe the behavior or changes that are being added by this MR.

-
-
-

## Does this introduce a breaking change?

- [ ] Yes
- [ ] No

If this introduces a breaking change, please describe the impact and migration path for existing applications below.


## Other information

Any other information that is important to this MR such as screenshots of how the component looks before and after the change.
