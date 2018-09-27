#!/usr/bin/python
"""\
@file   anim_tool.py
@author Brad Payne, Nat Goodspeed
@date   2015-09-15
@brief  This module contains tools for manipulating the .anim files supported
        for Second Life animation upload. Note that this format is unrelated
        to any non-Second Life formats of the same name.

        This code is a Python translation of the logic in
        LLKeyframeMotion::serialize() and deserialize():
        https://bitbucket.org/lindenlab/viewer-release/src/827a910542a9af0a39b0ca03663c02e5c83869ea/indra/llcharacter/llkeyframemotion.cpp?at=default&fileviewer=file-view-default#llkeyframemotion.cpp-1864
        https://bitbucket.org/lindenlab/viewer-release/src/827a910542a9af0a39b0ca03663c02e5c83869ea/indra/llcharacter/llkeyframemotion.cpp?at=default&fileviewer=file-view-default#llkeyframemotion.cpp-1220
        save that there is no support for old-style .anim files, permitting
        simpler code.

$LicenseInfo:firstyear=2015&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2015, Linden Research, Inc.

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

import math
import os
import random
from cStringIO import StringIO
import struct
import sys
from xml.etree import ElementTree

class Error(Exception):
    pass

class BadFormat(Error):
    """
    Something went wrong trying to read the specified .anim file.
    """
    pass

class ExtraneousData(BadFormat):
    """
    Specifically, the .anim file in question contains more data than needed.
    This could happen if the file isn't a .anim at all, and it 'just happens'
    to read properly otherwise -- e.g. a block of all zero bytes could look
    like empty name strings, empty arrays etc. That could be a legitimate
    error -- or it could be due to a sloppy tool. Break this exception out
    separately so caller can distinguish if desired.
    """
    pass

U16MAX = 65535
# One Over U16MAX, for scaling
OOU16MAX = 1.0/float(U16MAX)

LL_MAX_PELVIS_OFFSET = 5.0

class FilePacker(object):
    def __init__(self):
        self.buffer = StringIO()

    def write(self,filename):
        with open(filename,"wb") as f:
            f.write(self.buffer.getvalue())

    def pack(self,fmt,*args):
        buf = struct.pack(fmt, *args)
        self.buffer.write(buf)

    def pack_string(self,str,size=0):
        # If size == 0, caller doesn't care, just wants a terminating nul byte
        size = size or (len(str) + 1)
        # Nonzero size means a fixed-length field. If the passed string (plus
        # its terminating nul) exceeds that fixed length, we'll have to
        # truncate. But make sure we still leave room for the final nul byte!
        str = str[:size-1]
        # Now pad what's left of str out to 'size' with nul bytes.
        buf = str + ("\000" * (size-len(str)))
        self.buffer.write(buf)
        
class FileUnpacker(object):
    def __init__(self, filename):
        with open(filename,"rb") as f:
            self.buffer = f.read()
        self.offset = 0

    def unpack(self,fmt):
        result = struct.unpack_from(fmt, self.buffer, self.offset)
        self.offset += struct.calcsize(fmt)
        return result
    
    def unpack_string(self, size=0):
        # Nonzero size means we must consider exactly the next 'size'
        # characters in self.buffer.
        if size:
            self.offset += size
            # but stop at the first nul byte
            return self.buffer[self.offset-size:self.offset].split("\000", 1)[0]
        # Zero size means consider everything until the next nul character.
        result = self.buffer[self.offset:].split("\000", 1)[0]
        # don't forget to skip the nul byte too
        self.offset += len(result) + 1
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
        raise ValueError("U16 out of range: %s" % ival)
    val = ival*OOU16MAX
    delta = (upper - lower)
    val *= delta
    val += lower

    max_error = delta*OOU16MAX;

    # make sure that zeroes come through as zero
    if abs(val) < max_error:
        val = 0.0
    return val; 

class RotKey(object):
    def __init__(self, time, duration, rot):
        """
        This constructor instantiates a RotKey object from scratch, as it
        were, converting from float time to time_short.
        """
        self.time = time
        self.time_short = F32_to_U16(time, 0.0, duration) \
                          if time is not None else None
        self.rotation = rot

    @staticmethod
    def unpack(duration, fup):
        """
        This staticmethod constructs a RotKey by loadingfrom a FileUnpacker.
        """
        # cheat the other constructor
        this = RotKey(None, None, None)
        # load time_short directly from the file
        (this.time_short, ) = fup.unpack("<H")
        # then convert to float time
        this.time = U16_to_F32(this.time_short, 0.0, duration)
        # convert each coordinate of the rotation from short to float
        (x,y,z) = fup.unpack("<HHH")
        this.rotation = [U16_to_F32(i, -1.0, 1.0) for i in (x,y,z)]
        return this

    def dump(self, f):
        print >>f, "    rot_key: t %.3f" % self.time,"st",self.time_short,"rot",",".join("%.3f" % f for f in self.rotation)

    def pack(self, fp):
        fp.pack("<H",self.time_short)
        (x,y,z) = [F32_to_U16(v, -1.0, 1.0) for v in self.rotation]
        fp.pack("<HHH",x,y,z)
        
class PosKey(object):
    def __init__(self, time, duration, pos):
        """
        This constructor instantiates a PosKey object from scratch, as it
        were, converting from float time to time_short.
        """
        self.time = time
        self.time_short = F32_to_U16(time, 0.0, duration) \
                          if time is not None else None
        self.position = pos

    @staticmethod
    def unpack(duration, fup):
        """
        This staticmethod constructs a PosKey by loadingfrom a FileUnpacker.
        """
        # cheat the other constructor
        this = PosKey(None, None, None)
        # load time_short directly from the file
        (this.time_short, ) = fup.unpack("<H")
        # then convert to float time
        this.time = U16_to_F32(this.time_short, 0.0, duration)
        # convert each coordinate of the rotation from short to float
        (x,y,z) = fup.unpack("<HHH")
        this.position = [U16_to_F32(i, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET)
                         for i in (x,y,z)]
        return this

    def dump(self, f):
        print >>f, "    pos_key: t %.3f" % self.time,"pos ",",".join("%.3f" % f for f in self.position)
        
    def pack(self, fp):
        fp.pack("<H",self.time_short)
        (x,y,z) = [F32_to_U16(v, -LL_MAX_PELVIS_OFFSET, LL_MAX_PELVIS_OFFSET) for v in self.position]
        fp.pack("<HHH",x,y,z)

class Constraint(object):
    @staticmethod
    def unpack(duration, fup):
        this = Constraint()
        (this.chain_length, this.constraint_type) = fup.unpack("<BB")
        this.source_volume = fup.unpack_string(16)
        this.source_offset = fup.unpack("<fff")
        this.target_volume = fup.unpack_string(16)
        this.target_offset = fup.unpack("<fff")
        this.target_dir = fup.unpack("<fff")
        (this.ease_in_start, this.ease_in_stop, this.ease_out_start, this.ease_out_stop) = \
                             fup.unpack("<ffff")
        return this

    def pack(self, fp):
        fp.pack("<BB", self.chain_length, self.constraint_type)
        fp.pack_string(self.source_volume, 16)
        fp.pack("<fff", *self.source_offset)
        fp.pack_string(self.target_volume, 16)
        fp.pack("<fff", *self.target_offset)
        fp.pack("<fff", *self.target_dir)
        fp.pack("<ffff", self.ease_in_start, self.ease_in_stop,
                self.ease_out_start, self.ease_out_stop)

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
    @staticmethod
    def unpack(duration, fup):
        this = Constraints()
        (num_constraints, ) = fup.unpack("<i")
        this.constraints = [Constraint.unpack(duration, fup)
                            for i in xrange(num_constraints)]
        return this

    def pack(self, fp):
        fp.pack("<i",len(self.constraints))
        for c in self.constraints:
            c.pack(fp)

    def dump(self, f):
        print >>f, "constraints:",len(self.constraints)
        for c in self.constraints:
            c.dump(f)

class PositionCurve(object):
    def __init__(self):
        self.keys = []

    def is_static(self):
        if self.keys:
            k0 = self.keys[0]
            for k in self.keys:
                if k.position != k0.position:
                    return False
        return True

    @staticmethod
    def unpack(duration, fup):
        this = PositionCurve()
        (num_pos_keys, ) = fup.unpack("<i")
        this.keys = [PosKey.unpack(duration, fup)
                     for k in xrange(num_pos_keys)]
        return this

    def pack(self, fp):
        fp.pack("<i",len(self.keys))
        for k in self.keys:
            k.pack(fp)

    def dump(self, f):
        print >>f, "  position_curve:"
        print >>f, "    num_pos_keys", len(self.keys)
        for k in self.keys:
            k.dump(f)

class RotationCurve(object):
    def __init__(self):
        self.keys = []

    def is_static(self):
        if self.keys:
            k0 = self.keys[0]
            for k in self.keys:
                if k.rotation != k0.rotation:
                    return False
        return True

    @staticmethod
    def unpack(duration, fup):
        this = RotationCurve()
        (num_rot_keys, ) = fup.unpack("<i")
        this.keys = [RotKey.unpack(duration, fup)
                     for k in xrange(num_rot_keys)]
        return this

    def pack(self, fp):
        fp.pack("<i",len(self.keys))
        for k in self.keys:
            k.pack(fp)

    def dump(self, f):
        print >>f, "  rotation_curve:"
        print >>f, "    num_rot_keys", len(self.keys)
        for k in self.keys:
            k.dump(f)
            
class JointInfo(object):
    def __init__(self, name, priority):
        self.joint_name = name
        self.joint_priority = priority
        self.rotation_curve = RotationCurve()
        self.position_curve = PositionCurve()

    @staticmethod
    def unpack(duration, fup):
        this = JointInfo(None, None)
        this.joint_name = fup.unpack_string()
        (this.joint_priority, ) = fup.unpack("<i")
        this.rotation_curve = RotationCurve.unpack(duration, fup)
        this.position_curve = PositionCurve.unpack(duration, fup)
        return this

    def pack(self, fp):
        fp.pack_string(self.joint_name)
        fp.pack("<i", self.joint_priority)
        self.rotation_curve.pack(fp)
        self.position_curve.pack(fp)

    def dump(self, f):
        print >>f, "joint:"
        print >>f, "  joint_name:",self.joint_name
        print >>f, "  joint_priority:",self.joint_priority
        self.rotation_curve.dump(f)
        self.position_curve.dump(f)

class Anim(object):
    def __init__(self, filename=None, verbose=False):
        # set this FIRST as it's consulted by read() and unpack()
        self.verbose = verbose
        if filename:
            self.read(filename)

    def read(self, filename):
        fup = FileUnpacker(filename)
        try:
            self.unpack(fup)
        except struct.error as err:
            raise BadFormat("error reading %s: %s" % (filename, err))
        # By the end of streaming data in from our FileUnpacker, we should
        # have consumed the entire thing. If there's excess data, it's
        # entirely possible that this is a garbage file that happens to
        # resemble a valid degenerate .anim file, e.g. with zero counts of
        # things.
        if fup.offset != len(fup.buffer):
            raise ExtraneousData("extraneous data in %s; is it really a Linden .anim file?" %
                                 filename)

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

        # Also consult BVH conversion code for stricter checks

        # C++ deserialize() checks self.base_priority against
        # LLJoint::ADDITIVE_PRIORITY and LLJoint::USE_MOTION_PRIORITY,
        # possibly sets self.max_priority
        # checks self.duration against MAX_ANIM_DURATION !!
        # checks self.emote_name != str(self.ID)
        # checks self.hand_pose against LLHandMotion::NUM_HAND_POSES !!
        # checks 0 < num_joints <= LL_CHARACTER_MAX_JOINTS (no need --
        # validate names)
        # checks each joint_name neither "mScreen" nor "mRoot" ("attempted to
        # animate special joint") !!
        # checks each joint_name can be found in mCharacter
        # checks each joint_priority >= LLJoint::USE_MOTION_PRIORITY
        # tracks max observed joint_priority, excluding USE_MOTION_PRIORITY
        # checks each 0 <= RotKey.time <= self.duration !!
        # checks each RotKey.rotation.isFinite() !!
        # checks each PosKey.position.isFinite() !!
        # checks 0 <= num_constraints <= MAX_CONSTRAINTS  !!
        # checks each Constraint.chain_length <= num_joints
        # checks each Constraint.constraint_type < NUM_CONSTRAINT_TYPES !!
        # checks each Constraint.source_offset.isFinite() !!
        # checks each Constraint.target_offset.isFinite() !!
        # checks each Constraint.target_dir.isFinite() !!
        # from https://bitbucket.org/lindenlab/viewer-release/src/827a910542a9af0a39b0ca03663c02e5c83869ea/indra/llcharacter/llkeyframemotion.cpp?at=default&fileviewer=file-view-default#llkeyframemotion.cpp-1812 :
        # find joint to which each Constraint's collision volume is attached;
        # for each link in Constraint.chain_length, walk to joint's parent,
        # find that parent in list of joints, set its index in index list

        self.emote_name = fup.unpack_string()
        
        (self.loop_in_point, self.loop_out_point, self.loop,
         self.ease_in_duration, self.ease_out_duration, self.hand_pose, num_joints) = \
            fup.unpack("@ffiffII")
        
        self.joints = [JointInfo.unpack(self.duration, fup)
                       for j in xrange(num_joints)]
        if self.verbose:
            for joint_info in self.joints:
                print "unpacked joint",joint_info.joint_name
        self.constraints = Constraints.unpack(self.duration, fup)
        self.buffer = fup.buffer
        
    def pack(self, fp):
        fp.pack("@HHhf", self.version, self.sub_version, self.base_priority, self.duration)
        fp.pack_string(self.emote_name, 0)
        fp.pack("@ffiffII", self.loop_in_point, self.loop_out_point, self.loop,
                self.ease_in_duration, self.ease_out_duration, self.hand_pose, len(self.joints))
        for j in self.joints:
            j.pack(fp)
        self.constraints.pack(fp)

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
        print >>f, "num_joints", len(self.joints)
        for j in self.joints:
            j.dump(f)
        self.constraints.dump(f)
       
    def write(self, filename):
        fp = FilePacker()
        self.pack(fp)
        fp.write(filename)

    def write_src_data(self, filename):
        print "write file",filename
        with open(filename,"wb") as f:
            f.write(self.buffer)

    def find_joint(self, name):
        joints = [j for j in self.joints if j.joint_name == name]
        if joints:
            return joints[0]
        else:
            return None

    def add_joint(self, name, priority):
        if not self.find_joint(name):
            self.joints.append(JointInfo(name, priority))

    def delete_joint(self, name):
        j = self.find_joint(name)
        if j:
            if self.verbose:
                print "removing joint", name
            self.joints.remove(j)
        else:
            if self.verbose:
                print "joint not found to remove", name

    def summary(self):
        nj = len(self.joints)
        nz = len([j for j in self.joints if j.joint_priority > 0])
        nstatic = len([j for j in self.joints
                       if j.rotation_curve.is_static()
                       and j.position_curve.is_static()])
        print "summary: %d joints, non-zero priority %d, static %d" % (nj, nz, nstatic)

    def add_pos(self, joint_names, positions):
        js = [joint for joint in self.joints if joint.joint_name in joint_names]
        for j in js:
            if self.verbose:
                print "adding positions",j.joint_name,positions
            j.joint_priority = 4
            j.position_curve.keys = [PosKey(self.duration * i / (len(positions) - 1),
                                            self.duration,
                                            pos)
                                     for i,pos in enumerate(positions)]

    def add_rot(self, joint_names, rotations):
        js = [joint for joint in self.joints if joint.joint_name in joint_names]
        for j in js:
            print "adding rotations",j.joint_name
            j.joint_priority = 4
            j.rotation_curve.keys = [RotKey(self.duration * i / (len(rotations) - 1),
                                            self.duration,
                                            rot)
                                     for i,rot in enumerate(rotations)]

def twistify(anim, joint_names, rot1, rot2):
    js = [joint for joint in anim.joints if joint.joint_name in joint_names]
    for j in js:
        print "twisting",j.joint_name
        print len(j.rotation_curve.keys)
        j.joint_priority = 4
        # Set the joint(s) to rot1 at time 0, rot2 at the full duration.
        j.rotation_curve.keys = [
            RotKey(0.0, anim.duration, rot1),
            RotKey(anim.duration, anim.duration, rot2)]

def float_triple(arg):
    vals = arg.split()
    if len(vals)==3:
        return [float(x) for x in vals]
    else:
        raise ValueError("arg %s does not resolve to a float triple" % arg)

def get_joint_by_name(tree,name):
    if tree is None:
        return None
    matches = [elt for elt in tree.getroot().iter()
               if elt.get("name")==name
               and elt.tag in ["bone", "collision_volume", "attachment_point"]]
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

def resolve_joints(names, skel_tree, lad_tree, no_hud=False):
    print "resolve joints, no_hud is",no_hud
    if skel_tree and lad_tree:
        all_elts = [elt for elt in skel_tree.getroot().iter()]
        all_elts.extend([elt for elt in lad_tree.getroot().iter()])
        matches = set()
        for elt in all_elts:
            if elt.get("name") is None:
                continue
            #print elt.get("name"),"hud",elt.get("hud")
            if no_hud and elt.get("hud"):
                #print "skipping hud joint", elt.get("name")
                continue
            if elt.get("name") in names or elt.tag in names:
                matches.add(elt.get("name"))
        return list(matches)
    else:
        return names

def main(*argv):
    import argparse

    # default search location for config files is defined relative to
    # the script location; assuming they live in the same viewer repo
    # Use sys.argv[0] because (a) this script lives where it lives regardless
    # of what our caller passes and (b) we don't expect our caller to pass the
    # script name anyway.
    pathname = os.path.dirname(sys.argv[0])        
    # we're in scripts/content_tools; hop back to base of repository clone
    path_to_skel = os.path.join(os.path.abspath(pathname),os.pardir,os.pardir,
                                "indra","newview","character")

    parser = argparse.ArgumentParser(description="process SL animations")
    parser.add_argument("--verbose", help="verbose flag", action="store_true")
    parser.add_argument("--dump", help="dump to stdout", action="store_true")
    parser.add_argument("--rot", help="specify sequence of rotations", type=float_triple, nargs="+")
    parser.add_argument("--rand_pos", help="request NUM random positions (default %(default)s)",
                        metavar="NUM", type=int, default=2)
    parser.add_argument("--reset_pos", help="request original positions", action="store_true")
    parser.add_argument("--pos", help="specify sequence of positions", type=float_triple, nargs="+")
    parser.add_argument("--duration", help="specify duration", type=float)
    parser.add_argument("--loop_in", help="specify loop in time", type=float)
    parser.add_argument("--loop_out", help="specify loop out time", type=float)
    parser.add_argument("--num_pos", help="number of positions to create", type=int, default=2)
    parser.add_argument("--delete_joints", help="specify joints to be deleted", nargs="+",
                        metavar="JOINT")
    parser.add_argument("--joints", help="specify joints to be added or modified", nargs="+",
                        metavar="JOINT")
    parser.add_argument("--summary", help="print summary of the output animation", action="store_true")
    parser.add_argument("--skel", help="name of the avatar_skeleton file (default %(default)s)",
                        default=os.path.join(path_to_skel,"avatar_skeleton.xml"),
                        metavar="FILEPATH")
    parser.add_argument("--lad", help="name of the avatar_lad file (default %(default)s)",
                        default=os.path.join(path_to_skel,"avatar_lad.xml"),
                        metavar="FILEPATH")
    parser.add_argument("--set_version", nargs=2, type=int,
                        help="set version and sub-version to specified values",
                        metavar=("VERSION", "SUB-VERSION"))
    parser.add_argument("--no_hud", help="omit hud joints from list of attachments", action="store_true")
    parser.add_argument("--base_priority", help="set base priority", type=int)
    parser.add_argument("--joint_priority", help="set joint priority for all joints", type=int)
    parser.add_argument("infilename", help="name of a .anim file to input")
    parser.add_argument("outfilename", nargs="?", help="name of a .anim file to output")
    args = parser.parse_args(argv)

    print "anim_tool.py: " + " ".join(argv)
    print "dump is", args.dump
    print "infilename",args.infilename,"outfilename",args.outfilename
    print "rot",args.rot
    print "pos",args.pos
    print "joints",args.joints

    anim = Anim(args.infilename, args.verbose)
    skel_tree = None
    lad_tree = None
    joints = []
    if args.skel:
        skel_tree = ElementTree.parse(args.skel)
        if skel_tree is None:
            raise Error("failed to parse " + args.skel)
    if args.lad:
        lad_tree = ElementTree.parse(args.lad)
        if lad_tree is None:
            raise Error("failed to parse " + args.lad)
    if args.joints:
        joints = resolve_joints(args.joints, skel_tree, lad_tree, args.no_hud)
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
        # pick a random sequence of positions for each joint specified
        for joint in joints:
            # generate a list of rand_pos triples
            pos_array = [tuple(random.uniform(-1,1) for i in xrange(3))
                         for j in xrange(args.rand_pos)]
            # close the loop by cycling back to the first entry
            pos_array.append(pos_array[0])
            anim.add_pos([joint], pos_array)
    if joints and args.reset_pos:
        for joint in joints:
            elt = get_joint_by_name(skel_tree,joint) or get_joint_by_name(lad_tree,joint)
            if elt is not None:
                anim.add_pos([joint], 2*[get_elt_pos(elt)])
            else:
                print "no elt or no pos data for",joint
    if args.set_version:
        anim.version, anim.sub_version = args.set_version
    if args.base_priority is not None:
        print "set base priority",args.base_priority
        anim.base_priority = args.base_priority
    # --joint_priority sets priority for ALL joints, not just the explicitly-
    # specified ones
    if args.joint_priority is not None:
        print "set joint priority",args.joint_priority
        for joint in anim.joints:
            joint.joint_priority = args.joint_priority
    if args.duration is not None:
        print "set duration",args.duration
        anim.duration = args.duration
    if args.loop_in is not None:
        print "set loop_in",args.loop_in
        anim.loop_in_point = args.loop_in
    if args.loop_out is not None:
        print "set loop_out",args.loop_out
        anim.loop_out_point = args.loop_out
    if args.dump:
        anim.dump("-")
    if args.summary:
        anim.summary()
    if args.outfilename:
        anim.write(args.outfilename)

if __name__ == "__main__":
    try:
        sys.exit(main(*sys.argv[1:]))
    except Error as err:
        sys.exit("%s: %s" % (err.__class__.__name__, err))
