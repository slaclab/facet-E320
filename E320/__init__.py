# This file is part of E320

from . import _version
__version__ = _version.get_versions()['version']

print('This is E320 version {}'.format(__version__))
