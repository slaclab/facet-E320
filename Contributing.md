# Contributing to E320

## tl;dr

* Send pull requests as early as possible to ensure early community feedback. Put WIP (work-in-progess) in the title to show others you are still working on the changeset.
* Just send a pull request including your changes. The CI will run on any PR and automatic tests must pass. To speed things up, run tests on your machine by running the `run-tests.sh` script.
* write tests for the code you are adding! Other contributions may break your code unintentionally if your use cases are not covered by tests.
* try to reduce the number of packages your changes depend on.
* keep in mind that other users may have different use cases. Your changes should not interfere/break any other functionality (not everything is covered by tests, although it should be).
