#!/usr/bin/env python

## Copyright (c) 2016 Ioan Chera
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see http://www.gnu.org/licenses/
##


import os
cur_path = os.path.dirname(os.path.realpath(__file__))
bind_path = os.path.join(cur_path, '../../source/ev_actions.cpp')

with open(bind_path) as f:
	lines = f.readlines()

token = 0

entries = []

for full_line in lines:
	line = full_line.strip()
	if token == 0:
		if line == '//':
			token = 1
	elif token == 1:
		elems = line.split(' ')
		if len(elems) != 2:
			token = 0
		else:
			func_name = elems[1]
			token = 2
	elif token == 2:
		if line == '//':
			token = 3
		else:
			token = 0
	elif token == 3:
		elems = line.split(' ')
		if len(elems) < 3:
			token = 0
		else:
			if elems[1] != 'Implements':
				token = 0
			else:
				subelems = elems[2].split('(')
				if len(subelems) < 2:
					token = 0
				else:
					func_name = subelems[0]
		if token != 0:
			num_params = len(line.split(','))
			token = 4
	elif token == 4:
		if len(line) < 15:
			token = 0
		else:
			if line[5:14] != 'ExtraData':
				token = 0
			else:
				token = 5
	elif token == 5:
		subelems = line.split(':')
		if len(subelems) != 2:
			token = 0
		else:
			spec_num = subelems[1].strip()
			entries.append(' ' * (3 - len(spec_num)) + spec_num + ':' + func_name + '(' + str(num_params) + ')')
		token = 0
entries.sort()

print 'specials'
for entry in entries[:-1]:
	print '\t' + entry + ','
	
print '\t' + entries[-1] + ';'