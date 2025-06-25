#! /usr/bin/env python3
import os

if not os.path.exists('build'):
    os.mkdir('build')
os.chdir('build')
os.system('cmake -DENABLE_TEST=ON ..')
os.system('cmake --build .')
