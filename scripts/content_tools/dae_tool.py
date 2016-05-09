#!runpy.sh

"""\

This module contains tools for manipulating collada files

$LicenseInfo:firstyear=2016&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2016, Linden Research, Inc.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
version 2.1 of the License only.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
$/LicenseInfo$
"""

import argparse
import random

# Need to pip install numpy and pycollada
import numpy as np
from collada import *
from lxml import etree

def mesh_summary(mesh):
    print "scenes",mesh.scenes
    for scene in mesh.scenes:
        print "scene",scene
        for node in scene.nodes:
            print "node",node

def mesh_lock_offsets(tree, joints):
    print "mesh_lock_offsets",tree,joints
    for joint_node in tree.iter():
        if "node" not in joint_node.tag:
            continue
        if joint_node.get("type") != "JOINT":
            continue
        if joint_node.get("name") in joints or "bone" in joints:
            for matrix_node in list(joint_node):
                if "matrix" in matrix_node.tag:
                    floats = [float(x) for x in matrix_node.text.split()]
                    if len(floats) == 16:
                        floats[3] += 0.0001
                        floats[7] += 0.0001
                        floats[11] += 0.0001
                        matrix_node.text = " ".join([str(f) for f in floats])
                        print joint_node.get("name"),matrix_node.tag,"text",matrix_node.text,len(floats),floats
        

def mesh_random_offsets(tree, joints):
    print "mesh_random_offsets",tree,joints
    for joint_node in tree.iter():
        if "node" not in joint_node.tag:
            continue
        if joint_node.get("type") != "JOINT":
            continue
        if not joint_node.get("name"):
            continue
        if joint_node.get("name") in joints or "bone" in joints:
            for matrix_node in list(joint_node):
                if "matrix" in matrix_node.tag:
                    floats = [float(x) for x in matrix_node.text.split()]
                    print "randomizing",floats
                    if len(floats) == 16:
                        floats[3] += random.uniform(-1.0,1.0)
                        floats[7] += random.uniform(-1.0,1.0)
                        floats[11] += random.uniform(-1.0,1.0)
                        matrix_node.text = " ".join([str(f) for f in floats])
                        print joint_node.get("name"),matrix_node.tag,"text",matrix_node.text,len(floats),floats
        

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="process SL animations")
    parser.add_argument("--verbose", action="store_true",help="verbose flag")
    parser.add_argument("infilename", help="name of a collada (dae) file to input")
    parser.add_argument("outfilename", nargs="?", help="name of a collada (dae) file to output", default = None)
    parser.add_argument("--lock_offsets", nargs="+", help="tweak position of listed joints to lock their offsets")
    parser.add_argument("--random_offsets", nargs="+", help="random offset position for listed joints")
    parser.add_argument("--summary", action="store_true", help="print summary info about input file")
    args = parser.parse_args()

    mesh = None     
    tree = None

    if args.infilename:
        print "reading",args.infilename
        mesh = Collada(args.infilename)
        tree = etree.parse(args.infilename)

    if args.summary:
        print "summarizing",args.infilename
        mesh_summary(mesh)
        
    if args.lock_offsets:
        print "locking offsets for",args.lock_offsets
        mesh_lock_offsets(tree, args.lock_offsets)

    if args.random_offsets:
        print "adding random offsets for",args.random_offsets
        mesh_random_offsets(tree, args.random_offsets)

    if args.outfilename:
        print "writing",args.outfilename
        f = open(args.outfilename,"w")
        print >>f, etree.tostring(tree, pretty_print=True) #need update to get: , short_empty_elements=True)
    
