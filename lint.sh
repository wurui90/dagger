find . -iname '*.h' -o -iname '*.cpp' | xargs clang-format -i
black .
buildifier -r .
