***************************
Python interface
***************************

Python interface for FEASST includes the following:

feasst.i
=========

This is the SWIG interface file for wrapping C++ to Python.
Typically only developers would be interested in this file.
It is generated using a script because the order of including headers is important.

depend.py
=========

This script is only used for new documentation or plugins.
It generates feasst.i and [plugin]/doc/\*rst files.

test.py
========

Unittests for python interface.

