#! /usr/bin/env python3
import os
import argparse

parser = argparse.ArgumentParser(description='build')

build_type_group = parser.add_mutually_exclusive_group()
build_type_group.add_argument('-d', '--debug', dest='build_type', action='store_const', const='Debug', default='Release')
build_type_group.add_argument('-r', '--release', dest='build_type', action='store_const', const='Release', default='Release')

args = parser.parse_args()

if not os.path.exists('build'):
    os.mkdir('build')
os.chdir('build')
os.system(f'cmake -DCMAKE_BUILD_TYPE={args.build_type} -DENABLE_TEST=ON ..')
os.system('cmake --build .')
