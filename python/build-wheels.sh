#!/bin/bash
for pybin in python3.6 python3.7; do
$pybin setup.py bdist_wheel
done