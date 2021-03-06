# E320 Python package

This is  a collection of code for the FACET experiment E-320. However, anything related to data access and analysis at [FACET](https://facet-ii.slac.stanford.edu/) is welcome here.


# Contents and Use

1) To use the python routines, just type `import E320` from anywhere. No need to copy the files after the package is installed.

2) `E320.camviewer` contains a python camera view application. Run it like any other python package with  
`python -m E320.camviewer`  
from anywhere.

3) The cameraserver is written in C and there not installed with the package. It is located in `tools/vimbaserver`.


# Installation for Users

* Clone or download the repository and install it using
```
pip install --user .
```
within your python environment you intend using it in.

* Test your installation by trying to import the package with `import E320` from another directory.


# Installation for Developers

* Clone or download the repository and install it in editable mode using
```
pip install --user -e .
```
which will link the current directory into the python search path. Please read the [Contributing Guidelines](Contributing.md) if you whish to contribute to the code.

* Test your installation by trying to import the package with `import E320` from another directory.

* Run `./run-tests.sh` for automatic testing **before any commit**.
