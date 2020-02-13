#!runpy.sh

"""

Given a csv file of texture info, batch up textures with like
properties into files based on target template. The immediate point is
to make LSL files that operate on discrete batches of textures.

$LicenseInfo:firstyear=2019&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2019, Linden Research, Inc.

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
import sys
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import os

parser = argparse.ArgumentParser(description="batch up texture ids into target files")
parser.add_argument("--max_batches", type=int, help="how many different batches to make", default=1) 
parser.add_argument("-n", type=int, help="textures per batch", default=64) 
parser.add_argument("texture_file", help="name of texture file")
parser.add_argument("template", help="name of template file")
args = parser.parse_args()

def apply_replacements(chars, replace):
    for k, v in replace.items():
        #print "replace",k,v
        chars = chars.replace("{" + k + "}", v)
    return chars

df = pd.read_csv(args.texture_file, names = ["texture_id", "width", "height", "channels", "bytes_per_pixel"]);
print df.describe();
grouped = df.groupby(["width", "height", "channels"])
for res in [128, 256, 1024]:
    for channels in [3]:
        matches = grouped.get_group((res,res,channels))
        ids = matches["texture_id"]
        print "res", res, "channels", channels, "textures found", len(ids)

        for which_tes in ["diffuse", "normal", "specular", "materials"]:
            for bn in xrange(0,args.max_batches):
                if bn * (args.n + 1) < len(ids):
                    batch_ids = ids[bn * args.n : bn * args.n + args.n]
                    print bn, len(batch_ids)
                    tex_list = [ '"' + tex + '"' for tex in batch_ids ]
                    tex_str = ",\n".join(tex_list)
                    #print tex_str
                    (subs_file,ext) = os.path.splitext(args.template)
                    descr = which_tes + "_n_" + str(args.n) + "_res_" + str(res) + "x" + str(res) + "x" + str(channels) + "_batch_" + str(bn)
                    subs_file += "_" + descr + ext
                    obj_name = descr
                    replace_dict = {"TEXTURE_IDS": tex_str, "WHICH": which_tes, "NAME": obj_name}
                    print "write to", subs_file
                    with open(args.template,"r") as f:
                        template_string = f.read()
                        file_string = apply_replacements(template_string, replace_dict)
                        with open(subs_file,"w") as fout:
                            fout.write(file_string)
