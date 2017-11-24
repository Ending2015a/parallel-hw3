#!/usr/bin/python
f = open('log.txt', 'r')
o = open('sorted_log.txt', 'w')

lines = []
for line in f:
    lines.append(line)

lines.sort()

for line in lines:
    o.write(line)

f.close()
o.close()
