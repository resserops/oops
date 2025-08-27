#! /usr/bin/env python3
import os
import argparse

parser = argparse.ArgumentParser(description='build')
build_type_group = parser.add_mutually_exclusive_group()
build_type_group.add_argument('-d', '--debug', dest='build_type', action='store_const', const='Debug')
build_type_group.add_argument('-r', '--release', dest='build_type', action='store_const', const='Release')
parser.set_defaults(build_type='Release')
parser.add_argument('-a', '--asan', action='store_true')
parser.add_argument('-t', '--test', action='store_true')

args = parser.parse_args()

cmake_args = f'-DCMAKE_BUILD_TYPE={args.build_type} '
if args.asan:
    cmake_args += '-DENABLE_ASAN=ON '
if args.test:
    cmake_args += '-DENABLE_TEST=ON '

if not os.path.exists('build'):
    os.mkdir('build')
os.chdir('build')
os.system(f'cmake {cmake_args}..')
os.system('cmake --build .')
