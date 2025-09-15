#! /usr/bin/env bash
find module -type f -name "*.h" -o -name "*.cpp" -o -name "*.tpp" | xargs clang-format -i
