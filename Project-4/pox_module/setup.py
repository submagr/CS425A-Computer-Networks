#!/usr/bin/env python
'''Setuptools params'''

from setuptools import setup, find_packages

setup(
    name='CSE425',
    version='0.0.0',
    description='Network controller for IITK CSE425 Lab',
    author='CSE425 TA',
    author_email='nomail',
    packages=find_packages(exclude='test'),
    long_description="""\
Insert longer description here.
      """,
      classifiers=[
          "License :: OSI Approved :: GNU General Public License (GPL)",
          "Programming Language :: Python",
          "Development Status :: 1 - Planning",
          "Intended Audience :: Developers",
          "Topic :: Internet",
      ],
      keywords='IITK CSE425',
      license='GPL',
      install_requires=[
        'setuptools',
        'twisted',
        'ltprotocol', # David Underhill's nice Length-Type protocol handler
      ])

