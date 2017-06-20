#!/usr/bin/python

import os, sys

if len(sys.argv) != 2:
	print("Expected path to xwmfs mount as argument")
	sys.exit(1)

mount = sys.argv[1]

children = {

}

root = None

windows_dir = os.path.join( mount, "windows" )

for window in os.listdir(windows_dir):

	parent = os.path.join(windows_dir, window, "parent")
	parent = open(parent).read().strip()

	if parent == "0":
		root = window
		continue

	child_list = children.setdefault(parent, [])
	child_list.append(window)

def print_tree(node, level):

	try:
		name = os.path.join(windows_dir, node, "name")
		name = open(name).read().strip()
	except:
		name = "<unnamed>"

	print(level * '\t', node, ": ", name, sep = '')

	for child in children.get(node, []):
		print_tree(child, level + 1)

if root:
	print_tree(root, 0)
else:
	print("Couldn't determine tree, did you use --handle-pseudo-windows?")

