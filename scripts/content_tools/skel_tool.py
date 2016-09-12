#!runpy.sh

"""\

This module contains tools for manipulating and validating the avatar skeleton file.

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

from lxml import etree
 
def get_joint_names(tree):
    joints = [element.get('name') for element in tree.getroot().iter() if element.tag in ['bone','collision_volume']]
    print "joints:",joints
    return joints

def get_aliases(tree):
    aliases = {}
    alroot = tree.getroot()
    for element in alroot.iter():
        for key in element.keys():
            if key == 'aliases':
                name = element.get('name')
                val = element.get('aliases')
                aliases[name] = val
    return aliases
    
def fix_name(element):
    pass
        
def enforce_precision_rules(element):
    pass

def float_tuple(str, n=3):
    try:
        result = tuple(float(e) for e in str.split())
        if len(result)==n:
            return result
        else:
            print "tuple length wrong:", str,"gave",result,"wanted len",n,"got len",len(result)
            raise Exception()
    except:
        print "convert failed for:",str
        raise

def check_symmetry(name, field, vec1, vec2):
    if vec1[0] != vec2[0]:
        print name,field,"x match fail"
    if vec1[1] != -vec2[1]:
        print name,field,"y mirror image fail"
    if vec1[2] != vec2[2]:
        print name,field,"z match fail"

def enforce_symmetry(tree, element, field, fix=False):
    name = element.get("name")
    if not name:
        return
    if "Right" in name:
        left_name = name.replace("Right","Left")
        left_element = get_element_by_name(tree, left_name)
        pos = element.get(field)
        left_pos = left_element.get(field)
        pos_tuple = float_tuple(pos)
        left_pos_tuple = float_tuple(left_pos)
        check_symmetry(name,field,pos_tuple,left_pos_tuple)

def get_element_by_name(tree,name):
    if tree is None:
        return None
    matches = [elt for elt in tree.getroot().iter() if elt.get("name")==name] 
    if len(matches)==1:
        return matches[0]
    elif len(matches)>1:
        print "multiple matches for name",name
        return None
    else:
        return None

def list_skel_tree(tree):
    for element in tree.getroot().iter():
        if element.tag == "bone":
            print element.get("name"),"-",element.get("support")
    
def validate_child_order(tree, ogtree, fix=False):
    unfixable = 0

    #print "validate_child_order am failing for NO RAISIN!"
    #unfixable += 1

    tofix = set()
    for element in tree.getroot().iter():
        if element.tag != "bone":
            continue
        og_element = get_element_by_name(ogtree,element.get("name"))
        if og_element is not None:
            for echild,ochild in zip(list(element),list(og_element)):
                if echild.get("name") != ochild.get("name"):
                    print "Child ordering error, parent",element.get("name"),echild.get("name"),"vs",ochild.get("name")
                    if fix:
                        tofix.add(element.get("name"))
    children = {}
    for name in tofix:
        print "FIX",name
        element = get_element_by_name(tree,name)
        og_element = get_element_by_name(ogtree,name)
        children = []
        # add children matching the original joints first, in the same order
        for og_elt in list(og_element):
            elt = get_element_by_name(tree,og_elt.get("name"))
            if elt is not None:
                children.append(elt)
                print "b:",elt.get("name")
            else:
                print "b missing:",og_elt.get("name")
        # then add children that are not present in the original joints
        for elt in list(element):
            og_elt = get_element_by_name(ogtree,elt.get("name"))
            if og_elt is None:
                children.append(elt)
                print "e:",elt.get("name")
        # if we've done this right, we have a rearranged list of the same length
        if len(children)!=len(element):
            print "children",[e.get("name") for e in children]
            print "element",[e.get("name") for e in element]
            print "children changes for",name,", cannot reconcile"
        else:
            element[:] = children

    return unfixable

#   Checklist for the final file, started from SL-276:
# - new "end" attribute on all bones
# - new "connected" attribute on all bones
# - new "support" tag on all bones and CVs
# - aliases where appropriate for backward compatibility. rFoot and lFoot associated with mAnkle bones (not mFoot bones)
# - correct counts of bones and collision volumes in header
# - check all comments
# - old fields of old bones and CVs should be identical to their previous values.
# - old bones and CVs should retain their previous ordering under their parent, with new joints going later in any given child list
# - corresponding right and left joints should be mirror symmetric.
# - childless elements should be in short form (<bone /> instead of <bone></bone>)
# - digits of precision should be consistent (again, except for old joints)
# - new bones should have pos, pivot the same
def validate_skel_tree(tree, ogtree, reftree, fix=False):
    print "validate_skel_tree"
    (num_bones,num_cvs) = (0,0)
    unfixable = 0
    defaults = {"connected": "false", 
                "group": "Face"
                }
    for element in tree.getroot().iter():
        og_element = get_element_by_name(ogtree,element.get("name"))
        ref_element = get_element_by_name(reftree,element.get("name"))
        # Preserve values from og_file:
        for f in ["pos","rot","scale","pivot"]:
            if og_element is not None and og_element.get(f) and (str(element.get(f)) != str(og_element.get(f))):
                print element.get("name"),"field",f,"has changed:",og_element.get(f),"!=",element.get(f)
                if fix:
                    element.set(f, og_element.get(f))

        # Pick up any other fields that we can from ogtree and reftree
        fields = []
        if element.tag in ["bone","collision_volume"]:
            fields = ["support","group"]
        if element.tag == 'bone':
            fields.extend(["end","connected"])
        for f in fields:
            if not element.get(f):
                print element.get("name"),"missing required field",f
                if fix:
                    if og_element is not None and og_element.get(f):
                        print "fix from ogtree"
                        element.set(f,og_element.get(f))
                    elif ref_element is not None and ref_element.get(f):
                        print "fix from reftree"
                        element.set(f,ref_element.get(f))
                    else:
                        if f in defaults:
                            print "fix by using default value",f,"=",defaults[f]
                            element.set(f,defaults[f])
                        elif f == "support":
                            if og_element is not None:
                                element.set(f,"base")
                            else:
                                element.set(f,"extended")
                        else:
                            print "unfixable:",element.get("name"),"no value for field",f
                            unfixable += 1

        fix_name(element)
        enforce_precision_rules(element)
        for field in ["pos","pivot"]:
            enforce_symmetry(tree, element, field, fix)
        if element.get("support")=="extended":
            if element.get("pos") != element.get("pivot"):
                print "extended joint",element.get("name"),"has mismatched pos, pivot"
        

        if element.tag == "linden_skeleton":
            num_bones = int(element.get("num_bones"))
            num_cvs = int(element.get("num_collision_volumes"))
            all_bones = [e for e in tree.getroot().iter() if e.tag=="bone"]
            all_cvs = [e for e in tree.getroot().iter() if e.tag=="collision_volume"]
            if num_bones != len(all_bones):
                print "wrong bone count, expected",len(all_bones),"got",num_bones
                if fix:
                    element.set("num_bones", str(len(all_bones)))
            if num_cvs != len(all_cvs):
                print "wrong cv count, expected",len(all_cvs),"got",num_cvs
                if fix:
                    element.set("num_collision_volumes", str(len(all_cvs)))

    print "skipping child order code"
    #unfixable += validate_child_order(tree, ogtree, fix)

    if fix and (unfixable > 0):
        print "BAD FILE:", unfixable,"errs could not be fixed"
            

def slider_info(ladtree,skeltree):
    for param in ladtree.iter("param"):
        for skel_param in param.iter("param_skeleton"):
            bones = [b for b in skel_param.iter("bone")]
        if bones:
            print "param",param.get("name"),"id",param.get("id")
            value_min = float(param.get("value_min"))
            value_max = float(param.get("value_max"))
            neutral = 100.0 * (0.0-value_min)/(value_max-value_min)
            print "  neutral",neutral
            for b in bones:
                scale = float_tuple(b.get("scale","0 0 0"))
                offset = float_tuple(b.get("offset","0 0 0"))
                print "  bone", b.get("name"), "scale", scale, "offset", offset
                print "    scale",scale
                print "    offset",offset
                scale_min = [value_min * s for s in scale]
                scale_max = [value_max * s for s in scale]
                offset_min = [value_min * t for t in offset]
                offset_max = [value_max * t for t in offset]
                if (scale_min != scale_max):
                    print "    Scale MinX", scale_min[0]
                    print "    Scale MinY", scale_min[1]
                    print "    Scale MinZ", scale_min[2]
                    print "    Scale MaxX", scale_max[0]
                    print "    Scale MaxY", scale_max[1]
                    print "    Scale MaxZ", scale_max[2]
                if (offset_min != offset_max):
                    print "    Offset MinX", offset_min[0]
                    print "    Offset MinY", offset_min[1]
                    print "    Offset MinZ", offset_min[2]
                    print "    Offset MaxX", offset_max[0]
                    print "    Offset MaxY", offset_max[1]
                    print "    Offset MaxZ", offset_max[2]
    
# Check contents of avatar_lad file relative to a specified skeleton
def validate_lad_tree(ladtree,skeltree,orig_ladtree):
    print "validate_lad_tree"
    bone_names = [elt.get("name") for elt in skeltree.iter("bone")]
    bone_names.append("mScreen")
    bone_names.append("mRoot")
    cv_names = [elt.get("name") for elt in skeltree.iter("collision_volume")]
    #print "bones\n ","\n  ".join(sorted(bone_names))
    #print "cvs\n ","\n  ".join(sorted(cv_names))
    for att in ladtree.iter("attachment_point"):
        att_name = att.get("name")
        #print "attachment",att_name
        joint_name = att.get("joint")
        if not joint_name in bone_names:
            print "att",att_name,"linked to invalid joint",joint_name
    for skel_param in ladtree.iter("param_skeleton"):
        skel_param_id = skel_param.get("id")
        skel_param_name = skel_param.get("name")
        #if not skel_param_name and not skel_param_id:
        #    print "strange skel_param"
        #    print etree.tostring(skel_param)
        #    for k,v in skel_param.attrib.iteritems():
        #        print k,"->",v
        for bone in skel_param.iter("bone"):
            bone_name = bone.get("name")
            if not bone_name in bone_names:
                print "skel param references invalid bone",bone_name
                print etree.tostring(bone)
            bone_scale = float_tuple(bone.get("scale","0 0 0"))
            bone_offset = float_tuple(bone.get("offset","0 0 0"))
            param = bone.getparent().getparent()
            if bone_scale==(0, 0, 0) and bone_offset==(0, 0, 0):
                print "no-op bone",bone_name,"in param",param.get("id","-1")
            # check symmetry of sliders
            if "Right" in bone.get("name"):
                left_name = bone_name.replace("Right","Left")
                left_bone = None
                for b in skel_param.iter("bone"):
                    if b.get("name")==left_name:
                        left_bone = b
                if left_bone is None:
                    print "left_bone not found",left_name,"in",param.get("id","-1")
                else:
                    left_scale = float_tuple(left_bone.get("scale","0 0 0"))
                    left_offset = float_tuple(left_bone.get("offset","0 0 0"))
                    if left_scale != bone_scale:
                        print "scale mismatch between",bone_name,"and",left_name,"in param",param.get("id","-1")
                    param_id = int(param.get("id","-1"))
                    if param_id in [661]: # shear
                        expected_offset = tuple([bone_offset[0],bone_offset[1],-bone_offset[2]])
                    elif param_id in [30656, 31663, 32663]: # shift
                        expected_offset = bone_offset
                    else:
                        expected_offset = tuple([bone_offset[0],-bone_offset[1],bone_offset[2]])
                    if left_offset != expected_offset:
                        print "offset mismatch between",bone_name,"and",left_name,"in param",param.get("id","-1")
                    
    drivers = {}
    for driven_param in ladtree.iter("driven"):
        driver = driven_param.getparent().getparent()
        driven_id = driven_param.get("id")
        driver_id = driver.get("id")
        actual_param = next(param for param in ladtree.iter("param") if param.get("id")==driven_id)
        if not driven_id in drivers:
            drivers[driven_id] = set()
            drivers[driven_id].add(driver_id)
            if (actual_param.get("value_min") != driver.get("value_min") or \
                actual_param.get("value_max") != driver.get("value_max")):
                if args.verbose:
                    print "MISMATCH min max:",driver.get("id"),"drives",driven_param.get("id"),"min",driver.get("value_min"),actual_param.get("value_min"),"max",driver.get("value_max"),actual_param.get("value_max")

    for driven_id in drivers:
        dset = drivers[driven_id]
        if len(dset) != 1:
            print "driven_id",driven_id,"has multiple drivers",dset
        else:
            if args.verbose:
                print "driven_id",driven_id,"has one driver",dset
    if orig_ladtree:
        # make sure expected message format is unchanged
        orig_message_params_by_id = dict((int(param.get("id")),param) for param in orig_ladtree.iter("param") if param.get("group") in ["0","3"])
        orig_message_ids = sorted(orig_message_params_by_id.keys())
        #print "orig_message_ids",orig_message_ids
        message_params_by_id = dict((int(param.get("id")),param) for param in ladtree.iter("param") if param.get("group") in ["0","3"])
        message_ids = sorted(message_params_by_id.keys())
        #print "message_ids",message_ids
        if (set(message_ids) != set(orig_message_ids)):
            print "mismatch in message ids!"
            print "added",set(message_ids) - set(orig_message_ids)
            print "removed",set(orig_message_ids) - set(message_ids)
        else:
            print "message ids OK"
    
def remove_joint_by_name(tree, name):
    print "remove joint:",name
    elt = get_element_by_name(tree,name)
    while elt is not None:
        children = list(elt)
        parent = elt.getparent()
        print "graft",[e.get("name") for e in children],"into",parent.get("name")
        print "remove",elt.get("name")
        #parent_children = list(parent)
        loc = parent.index(elt)
        parent[loc:loc+1] = children
        elt[:] = []
        print "parent now:",[e.get("name") for e in list(parent)]
        elt = get_element_by_name(tree,name)
    
def compare_skel_trees(atree,btree):
    diffs = {}
    realdiffs = {}
    a_missing = set()
    b_missing = set()
    a_names = set(e.get("name") for e in atree.getroot().iter() if e.get("name"))
    b_names = set(e.get("name") for e in btree.getroot().iter() if e.get("name"))
    print "a_names\n  ",str("\n  ").join(sorted(list(a_names)))
    print
    print "b_names\n  ","\n  ".join(sorted(list(b_names)))
    all_names = set.union(a_names,b_names)
    for name in all_names:
        if not name:
            continue
        a_element = get_element_by_name(atree,name)
        b_element = get_element_by_name(btree,name)
        if a_element is None or b_element is None:
            print "something not found for",name,a_element,b_element
        if a_element is not None and b_element is not None:
            all_attrib = set.union(set(a_element.attrib.keys()),set(b_element.attrib.keys()))
            print name,all_attrib
            for att in all_attrib:
                if a_element.get(att) != b_element.get(att):
                    if not att in diffs:
                        diffs[att] = set()
                    diffs[att].add(name)
                print "tuples",name,att,float_tuple(a_element.get(att)),float_tuple(b_element.get(att))
                if float_tuple(a_element.get(att)) != float_tuple(b_element.get(att)):
                    print "diff in",name,att
                    if not att in realdiffs:
                        realdiffs[att] = set()
                    realdiffs[att].add(name)
    for att in diffs:
        print "Differences in",att
        for name in sorted(diffs[att]):
            print "  ",name
    for att in realdiffs:
        print "Real differences in",att
        for name in sorted(diffs[att]):
            print "  ",name
    a_missing = b_names.difference(a_names)
    b_missing = a_names.difference(b_names)
    if len(a_missing) or len(b_missing):
        print "Missing from comparison"
        for name in a_missing:
            print "  ",name
        print "Missing from infile"
        for name in b_missing:
            print "  ",name

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="process SL avatar_skeleton/avatar_lad files")
    parser.add_argument("--verbose", action="store_true",help="verbose flag")
    parser.add_argument("--ogfile", help="specify file containing base bones", default="avatar_skeleton_orig.xml")
    parser.add_argument("--ref_file", help="specify another file containing replacements for missing fields")
    parser.add_argument("--lad_file", help="specify avatar_lad file to check", default="avatar_lad.xml")
    parser.add_argument("--orig_lad_file", help="specify avatar_lad file to compare to", default="avatar_lad_orig.xml")
    parser.add_argument("--aliases", help="specify file containing bone aliases")
    parser.add_argument("--validate", action="store_true", help="check specified input file for validity")
    parser.add_argument("--fix", action="store_true", help="try to correct errors")
    parser.add_argument("--remove", nargs="+", help="remove specified joints")
    parser.add_argument("--list", action="store_true", help="list joint names")
    parser.add_argument("--compare", help="alternate skeleton file to compare")
    parser.add_argument("--slider_info", help="information about the lad file sliders and affected bones", action="store_true")
    parser.add_argument("infilename", help="name of a skel .xml file to input", default="avatar_skeleton.xml")
    parser.add_argument("outfilename", nargs="?", help="name of a skel .xml file to output")
    args = parser.parse_args()

    tree = etree.parse(args.infilename)

    aliases = {}
    if args.aliases:
        altree = etree.parse(args.aliases)
        aliases = get_aliases(altree)

    # Parse input files
    ogtree = None
    reftree = None
    ladtree = None
    orig_ladtree = None

    if args.ogfile:
        ogtree = etree.parse(args.ogfile)

    if args.ref_file:
        reftree = etree.parse(args.ref_file)

    if args.lad_file:
        ladtree = etree.parse(args.lad_file)

    if args.orig_lad_file:
        orig_ladtree = etree.parse(args.orig_lad_file)

    if args.remove:
        for name in args.remove:
            remove_joint_by_name(tree,name)

    # Do processing
    if args.validate and ogtree:
        validate_skel_tree(tree, ogtree, reftree)

    if args.validate and ladtree:
        validate_lad_tree(ladtree, tree, orig_ladtree)

    if args.fix and ogtree:
        validate_skel_tree(tree, ogtree, reftree, True)

    if args.list and tree:
        list_skel_tree(tree)

    if args.compare and tree:
        compare_tree = etree.parse(args.compare)
        compare_skel_trees(compare_tree,tree)

    if ladtree and tree and args.slider_info:
        slider_info(ladtree,tree)
        
    if args.outfilename:
        f = open(args.outfilename,"w")
        print >>f, etree.tostring(tree, pretty_print=True) #need update to get: , short_empty_elements=True)

