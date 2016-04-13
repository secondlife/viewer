#!runpy.sh

"""\

This module contains tools for manipulating the .anim files supported
for Second Life animation upload. Note that this format is unrelated
to any non-Second Life formats of the same name.

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

import sys
import struct
import StringIO
import math
import argparse
import random
from lxml import etree

U16MAX = 65535
OOU16MAX = 1.0/(float)(U16MAX)

LL_MAX_PELVIS_OFFSET = 5.0

class FilePacker(object):
    def __init__(self):
        self.data = StringIO.StringIO()
        self.offset = 0

    def write(self,filename):
        f = open(filename,"wb")
        f.write(self.data.getvalue())
        f.close()

    def pack(self,fmt,*args):
        buf = struct.pack(fmt, *args)
        self.offset += struct.calcsize(fmt)
        self.data.write(buf)

    def pack_string(self,str,size=0):
        buf = str + "\000"
        if size and (len(buf) < size):
            buf += "\000" * (size-len(buf))
        self.data.write(buf)
        
class FileUnpacker(object):
    def __init__(self, filename):
        f = open(filename,"rb")
        self.data = f.read()
        self.offset = 0

    def unpack(self,fmt):
        result = struct.unpack_from(fmt, self.data, self.offset)
        self.offset += struct.calcsize(fmt)
        return result
    
    def unpack_string(self, size=0):
        result = ""
        i = 0
        while (self.data[self.offset+i] != "\000"):
            result += self.data[self.offset+i]
            i += 1
        i += 1
        if size:
            # fixed-size field for the string
            i = size
        self.offset += i
        return result

# translated from the C++ version in lldefs.h
def llclamp(a, minval, maxval):
    if a<minval:
        return minval
    if a>maxval:
        return maxval
    return a

# translated from the C++ version in llquantize.h
def F32_to_U16(val, lower, upper):
    val = llclamp(val, lower, upper);
    # make sure that the value is positive and normalized to <0, 1>
    val -= lower;
    val /= (upper - lower);
    
    # return the U16
    return int(math.floor(val*U16MAX))

# translated from the C++ version in llquantize.h
def U16_to_F32(ival, lower, upper):
    if ival < 0 or ival > U16MAX:
        raise Exception("U16 out of range: "+ival)
    val = ival*OOU16MAX
    delta = (upper - lower)
    val *= delta
    val += lower

    max_error = delta*OOU16MAX;

    # make sure that zeroes come through as zero
    if abs(val) < max_error:
        val = 0.0
    return val; 

class BadFormat(Exception):
    pass

class RotKey(object):
    def __init__(self):
        pass

    def unpack(self, anim, fup):
        (self.time_short, ) = fup.unpack("<H")
        self.time = U16_to_F32(self.time_short, 0.0, anim.duration)
        (x,y,z) = fup.unpack("<HHH")
        self.rotation = [U16_to_F32(i, -1.0, 1.0) for i in (x,y,z)]

    def dump(self, f):
        print >>f, "    rot_key: t",self.time,"st",self.time_short,"rot",",".join([str(f) for f in self.rotation])

    def pack(self, anim, fp):
        if not hasattr(self,"time_short"):
            self.time_short = F32_to_U16(self.time, 0.0, anim.duration)
        fp.pack("<H",self.time_short)
        (x,y,z) = [F32_to_U16(v, -1.0, 1.0) for v in self.rotation]
        fp.pack("<HHH",x,y,z)
        
class PosKey(object):
    def __init__(self):
        pass

    def unpack(self, anim, fup):
        (self.time_short, ) = fup.unpack("<H")
        self.time = U16_to_F32(self.time_short, 0.0, anim.duration)
        (x,y,z) = fup.unpack("<HHH")
        self.position = [U16_to_F32(i, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET) for i in (x,y,z)]

    def dump(self, f):
        print >>f, "    pos_key: t",self.time,"pos ",",".join([str(f) for f in self.position])
        
    def pack(self, anim, fp):
        if not hasattr(self,"time_short"):
            self.time_short = F32_to_U16(self.time, 0.0, anim.duration)
        fp.pack("<H",self.time_short)
        (x,y,z) = [F32_to_U16(v, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET) for v in self.position]
        fp.pack("<HHH",x,y,z)

class Constraint(object):
    def __init__(self):
        pass

    def unpack(self, anim, fup):
        (self.chain_length, self.constraint_type) = fup.unpack("<BB")
        self.source_volume = fup.unpack_string(16)
        self.source_offset = fup.unpack("<fff")
        self.target_volume = fup.unpack_string(16)
        self.target_offset = fup.unpack("<fff")
        self.target_dir = fup.unpack("<fff")
        fmt = "<ffff"
        (self.ease_in_start, self.ease_in_stop, self.ease_out_start, self.ease_out_stop) = fup.unpack("<ffff")

    def pack(self, anim, fp):
        fp.pack("<BB", self.chain_length, self.constraint_type)
        fp.pack_string(self.source_volume, 16)
        fp.pack("<fff", *self.source_offset)
        fp.pack_string(self.target_volume, 16)
        fp.pack("<fff", *self.target_offset)
        fp.pack("<fff", *self.target_dir)
        fp.pack("<ffff", self.ease_in_start, self.ease_in_stop, self.ease_out_start, self.ease_out_stop)

    def dump(self, f):
        print >>f, "  constraint:"
        print >>f, "    chain_length",self.chain_length
        print >>f, "    constraint_type",self.constraint_type
        print >>f, "    source_volume",self.source_volume
        print >>f, "    source_offset",self.source_offset
        print >>f, "    target_volume",self.target_volume
        print >>f, "    target_offset",self.target_offset
        print >>f, "    target_dir",self.target_dir
        print >>f, "    ease_in_start",self.ease_in_start
        print >>f, "    ease_in_stop",self.ease_in_stop
        print >>f, "    ease_out_start",self.ease_out_start
        print >>f, "    ease_out_stop",self.ease_out_stop
        
class Constraints(object):
    def __init__(self):
        pass

    def unpack(self, anim, fup):
        (self.num_constraints, ) = fup.unpack("<i")
        self.constraints = []
        for i in xrange(self.num_constraints):
            constraint = Constraint()
            constraint.unpack(anim, fup)
            self.constraints.append(constraint)

    def pack(self, anim, fp):
        fp.pack("<i",self.num_constraints)
        for c in self.constraints:
            c.pack(anim,fp)

    def dump(self, f):
        print >>f, "constraints:",self.num_constraints
        for c in self.constraints:
            c.dump(f)

class PositionCurve(object):
    def __init__(self):
        self.num_pos_keys = 0
        self.keys = []

    def is_static(self):
        if self.keys:
            k0 = self.keys[0]
            for k in self.keys:
                if k.position != k0.position:
                    return False
        return True

    def unpack(self, anim, fup):
        (self.num_pos_keys, ) = fup.unpack("<i")
        self.keys = []
        for k in xrange(0,self.num_pos_keys):
            pos_key = PosKey()
            pos_key.unpack(anim, fup)
            self.keys.append(pos_key)

    def pack(self, anim, fp):
        fp.pack("<i",self.num_pos_keys)
        for k in self.keys:
            k.pack(anim, fp)

    def dump(self, f):
        print >>f, "  position_curve:"
        print >>f, "    num_pos_keys", self.num_pos_keys
        for k in xrange(0,self.num_pos_keys):
            self.keys[k].dump(f)

class RotationCurve(object):
    def __init__(self):
        self.num_rot_keys = 0
        self.keys = []

    def is_static(self):
        if self.keys:
            k0 = self.keys[0]
            for k in self.keys:
                if k.rotation != k0.rotation:
                    return False
        return True

    def unpack(self, anim, fup):
        (self.num_rot_keys, ) = fup.unpack("<i")
        self.keys = []
        for k in xrange(0,self.num_rot_keys):
            rot_key = RotKey()
            rot_key.unpack(anim, fup)
            self.keys.append(rot_key)

    def pack(self, anim, fp):
        fp.pack("<i",self.num_rot_keys)
        for k in self.keys:
            k.pack(anim, fp)

    def dump(self, f):
        print >>f, "  rotation_curve:"
        print >>f, "    num_rot_keys", self.num_rot_keys
        for k in xrange(0,self.num_rot_keys):
            self.keys[k].dump(f)
            
class JointInfo(object):
    def __init__(self):
        pass

    def unpack(self, anim, fup):
        self.joint_name = fup.unpack_string()
        (self.joint_priority, ) = fup.unpack("<i")
        self.rotation_curve = RotationCurve()
        self.rotation_curve.unpack(anim, fup)
        self.position_curve = PositionCurve()
        self.position_curve.unpack(anim, fup)

    def pack(self, anim, fp):
        fp.pack_string(self.joint_name)
        fp.pack("<i", self.joint_priority)
        self.rotation_curve.pack(anim, fp)
        self.position_curve.pack(anim, fp)

    def dump(self, f):
        print >>f, "joint:"
        print >>f, "  joint_name:",self.joint_name
        print >>f, "  joint_priority:",self.joint_priority
        self.rotation_curve.dump(f)
        self.position_curve.dump(f)

class Anim(object):
    def __init__(self, filename=None):
        if filename:
            self.read(filename)

    def read(self, filename):
        fup = FileUnpacker(filename)
        self.unpack(fup)

    # various validity checks could be added - see LLKeyframeMotion::deserialize()
    def unpack(self,fup):
        (self.version, self.sub_version, self.base_priority, self.duration) = fup.unpack("@HHhf")

        if self.version == 0 and self.sub_version == 1:
            self.old_version = True
            raise BadFormat("old version not supported")
        elif self.version == 1 and self.sub_version == 0:
            self.old_version = False
        else:
            raise BadFormat("Bad combination of version, sub_version: %d %d" % (self.version, self.sub_version))

        self.emote_name = fup.unpack_string()
        
        (self.loop_in_point, self.loop_out_point, self.loop, self.ease_in_duration, self.ease_out_duration, self.hand_pose, self.num_joints) = fup.unpack("@ffiffII")
        
        self.joints = []
        for j in xrange(0,self.num_joints):
            joint_info = JointInfo()
            joint_info.unpack(self, fup)
            self.joints.append(joint_info)
            print "unpacked joint",joint_info.joint_name
        self.constraints = Constraints()
        self.constraints.unpack(self, fup)
        self.data = fup.data
        
    def pack(self, fp):
        fp.pack("@HHhf", self.version, self.sub_version, self.base_priority, self.duration)
        fp.pack_string(self.emote_name, 0)
        fp.pack("@ffiffII", self.loop_in_point, self.loop_out_point, self.loop, self.ease_in_duration, self.ease_out_duration, self.hand_pose, self.num_joints)
        for j in self.joints:
            j.pack(anim, fp)
        self.constraints.pack(anim, fp)

    def dump(self, filename="-"):
        if filename=="-":
            f = sys.stdout
        else:
            f = open(filename,"w")
        print >>f, "versions: ", self.version, self.sub_version
        print >>f, "base_priority: ", self.base_priority
        print >>f, "duration: ", self.duration
        print >>f, "emote_name: ", self.emote_name
        print >>f, "loop_in_point: ", self.loop_in_point
        print >>f, "loop_out_point: ", self.loop_out_point
        print >>f, "loop: ", self.loop
        print >>f, "ease_in_duration: ", self.ease_in_duration
        print >>f, "ease_out_duration: ", self.ease_out_duration
        print >>f, "hand_pose", self.hand_pose
        print >>f, "num_joints", self.num_joints
        for j in self.joints:
            j.dump(f)
        self.constraints.dump(f)
       
    def write(self, filename):
        fp = FilePacker()
        self.pack(fp)
        fp.write(filename)

    def write_src_data(self, filename):
        print "write file",filename
        f = open(filename,"wb")
        f.write(self.data)
        f.close()
        
    def find_joint(self, name):
        joints = [j for j in self.joints if j.joint_name == name]
        if joints:
            return joints[0]
        else:
            return None

    def add_joint(self, name, priority):
        if not self.find_joint(name):
            j = JointInfo()
            j.joint_name = name
            j.joint_priority = priority
            j.rotation_curve = RotationCurve()
            j.position_curve = PositionCurve()
            self.joints.append(j)
            self.num_joints = len(self.joints)

    def delete_joint(self, name):
        j = self.find_joint(name)
        if j:
            anim.joints.remove(j)
            anim.num_joints = len(self.joints)

    def summary(self):
        nj = len(self.joints)
        nz = len([j for j in self.joints if j.joint_priority > 0])
        nstatic = len([j for j in self.joints if j.rotation_curve.is_static() and j.position_curve.is_static()])
        print "summary: %d joints, non-zero priority %d, static %d" % (nj, nz, nstatic)

    def add_pos(self, joint_names, positions):
        js = [joint for joint in self.joints if joint.joint_name in joint_names]
        for j in js:
            if args.verbose:
                print "adding positions",j.joint_name,positions
            j.joint_priority = 4
            j.position_curve.num_pos_keys = len(positions)
            j.position_curve.keys = []
            for i,pos in enumerate(positions):
                key = PosKey()
                key.time = self.duration * i / (len(positions) - 1)
                key.time_short = F32_to_U16(key.time, 0.0, self.duration)
                key.position = pos
                j.position_curve.keys.append(key)

    def add_rot(self, joint_names, rotations):
        js = [joint for joint in self.joints if joint.joint_name in joint_names]
        for j in js:
            print "adding rotations",j.joint_name
            j.joint_priority = 4
            j.rotation_curve.num_rot_keys = len(rotations)
            j.rotation_curve.keys = []
            for i,pos in enumerate(rotations):
                key = RotKey()
                key.time = self.duration * i / (len(rotations) - 1)
                key.time_short = F32_to_U16(key.time, 0.0, self.duration)
                key.rotation = pos
                j.rotation_curve.keys.append(key)

def twistify(anim, joint_names, rot1, rot2):
    js = [joint for joint in anim.joints if joint.joint_name in joint_names]
    for j in js:
        print "twisting",j.joint_name
        print j.rotation_curve.num_rot_keys
        j.joint_priority = 4
        j.rotation_curve.num_rot_keys = 2
        j.rotation_curve.keys = []
        key1 = RotKey()
        key1.time_short = 0
        key1.time = U16_to_F32(key1.time_short, 0.0, anim.duration)
        key1.rotation = rot1
        key2 = RotKey()
        key2.time_short = U16MAX
        key2.time = U16_to_F32(key2.time_short, 0.0, anim.duration)
        key2.rotation = rot2
        j.rotation_curve.keys.append(key1)
        j.rotation_curve.keys.append(key2)

def float_triple(arg):
    vals = arg.split()
    if len(vals)==3:
        return [float(x) for x in vals]
    else:
        raise Exception("arg %s does not resolve to a float triple" % arg)

def get_joint_by_name(tree,name):
    if tree is None:
        return None
    matches = [elt for elt in tree.getroot().iter() if \
                   elt.get("name")==name and elt.tag in ["bone", "collision_volume", "attachment_point"]]
    if len(matches)==1:
        return matches[0]
    elif len(matches)>1:
        print "multiple matches for name",name
        return None
    else:
        return None

def get_elt_pos(elt):
    if elt.get("pos"):
        return float_triple(elt.get("pos"))
    elif elt.get("position"):
        return float_triple(elt.get("position"))
    else:
        return (0.0, 0.0, 0.0)

def resolve_joints(names, skel_tree, lad_tree):
    if skel_tree and lad_tree:
        matches = [element.get("name") for element in skel_tree.getroot().iter() if (element.get("name") in names) or (element.tag in names)]
        matches.extend([element.get("name") for element in lad_tree.getroot().iter() if (element.get("name") in names) or (element.tag in names)])
        return matches
    else:
        return names

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="process SL animations")
    parser.add_argument("--verbose", help="verbose flag", action="store_true")
    parser.add_argument("--dump", help="dump to specified file")
    parser.add_argument("--rot", help="specify sequence of rotations", type=float_triple, nargs="+")
    parser.add_argument("--rand_pos", help="request random positions", action="store_true")
    parser.add_argument("--reset_pos", help="request original positions", action="store_true")
    parser.add_argument("--pos", help="specify sequence of positions", type=float_triple, nargs="+")
    parser.add_argument("--delete_joints", help="specify joints to be deleted", nargs="+")
    parser.add_argument("--joints", help="specify joints to be added or modified", nargs="+")
    parser.add_argument("--summary", help="print summary of the output animation", action="store_true")
    parser.add_argument("--skel", help="name of the avatar_skeleton file", default="avatar_skeleton.xml")
    parser.add_argument("--lad", help="name of the avatar_lad file", default="avatar_lad.xml")
    parser.add_argument("--set_version", nargs=2, type=int, help="set version and sub-version to specified values")
    parser.add_argument("infilename", help="name of a .anim file to input")
    parser.add_argument("outfilename", nargs="?", help="name of a .anim file to output")
    args = parser.parse_args()

    print "anim_tool.py: " + " ".join(sys.argv)
    print "dump is", args.dump
    print "infilename",args.infilename,"outfilename",args.outfilename
    print "rot",args.rot
    print "pos",args.pos
    print "joints",args.joints

    try:
        anim = Anim(args.infilename)
        skel_tree = None
        lad_tree = None
        joints = []
        if args.skel:
            skel_tree = etree.parse(args.skel)
            if skel_tree is None:
                print "failed to parse",args.skel
                exit(1)
        if args.lad:
            lad_tree = etree.parse(args.lad)
            if lad_tree is None:
                print "failed to parse",args.lad
                exit(1)
        if args.joints:
            joints = resolve_joints(args.joints, skel_tree, lad_tree)
            if args.verbose:
                print "joints resolved to",joints
            for name in joints:
                anim.add_joint(name,0)
        if args.delete_joints:
            for name in args.delete_joints:
                anim.delete_joint(name)
        if joints and args.rot:
            anim.add_rot(joints, args.rot)
        if joints and args.pos:
            anim.add_pos(joints, args.pos)
        if joints and args.rand_pos:
            for joint in joints:
                pos_array = list(tuple(random.uniform(-1,1) for i in xrange(3)) for j in xrange(2))
                pos_array.append(pos_array[0])
                anim.add_pos([joint], pos_array)
        if joints and args.reset_pos:
            for joint in joints:
                elt = get_joint_by_name(skel_tree,joint)
                if elt is None:
                    elt = get_joint_by_name(lad_tree,joint)
                if elt is not None:
                    pos_array = []
                    pos_array.append(get_elt_pos(elt))
                    pos_array.append(pos_array[0])
                    anim.add_pos([joint], pos_array)
                else:
                    print "no elt or no pos data for",joint
        if args.set_version:
            anim.version = args.set_version[0]
            anim.sub_version = args.set_version[1]
        if args.dump:
            anim.dump(args.dump)
        if args.summary:
            anim.summary()
        if args.outfilename:
            anim.write(args.outfilename)
    except:
        raise

