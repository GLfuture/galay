find . -regex ".*\.\(cc\|h\|hpp\)" -type f -print0 | xargs -0 wc -l