#!/bin/bash

set -eux

# test codestyle
python -m pycodestyle E320 --statistics --count --show-source --ignore=W391,E123,E226,E24,W504 --max-line-length=99
