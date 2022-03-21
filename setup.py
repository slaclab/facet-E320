#!/usr/bin/env python3


from setuptools import setup, find_packages
import versioneer


setup(name='E320',
      version=versioneer.get_version(),
      cmdclass=versioneer.get_cmdclass(),
      packages=find_packages(include=['E320*']))
