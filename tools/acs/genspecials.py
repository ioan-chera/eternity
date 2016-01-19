#!/usr/bin/env python
#
# Copyright (c) 2016 Ioan Chera
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see http://www.gnu.org/licenses/
#

import os
import re

cur_path = os.path.dirname(os.path.realpath(__file__))
bindings_path = os.path.join(cur_path, '../../source/ev_bindings.cpp')
actions_path = os.path.join(cur_path, '../../source/ev_actions.cpp')

with open(bindings_path) as f:
	lines = f.readlines()

dict = {}

in_hexen = False
for full_line in lines:
	line = full_line.strip()
	if not in_hexen:
		tokens = line.split(' ')
		if len(tokens) < 2:
			continue
		if tokens[0] == 'ev_binding_t' and tokens[1].startswith('HexenBindings'):
			in_hexen = True
			continue
	else:
		if line == '{':
			continue
		if not line.startswith('LINESPECNAMED'):
			in_hexen = False
			break
		# it starts with that
		number = re.search(r'[0-9]+', line).group()
		identifier = re.search(r'[A-Za-z0-9]+', line[line.index(','):]).group()
		name = re.search(r'\"[A-Za-z_0-9]+\"', line).group()
		print number, identifier, name[1:-1]
		dict[name[1:-1]] = number

with open(actions_path) as f:
	lines = f.readlines()

print 'special'
for full_line in lines:
	line = full_line.strip()
	if not line.startswith('//'):
		continue
	a = re.search(r'implements', line.lower())
	if a is None:
		continue
	name = re.search(r'[A-Za-z0-9_]+', line[a.end():])
	if name is None:
		continue
	name = name.group()
	if name not in dict:
		continue
	b = re.search(r'\(.*\)', line)
	if b is None:
		continue
	b = b.group().split(',')
	num_args = len(b)
	print '\t' + dict[name] + ':' + name + '(' + str(num_args) + ')'