#!/usr/bin/env python3
"""\
This script for plotting results of llik_test.cpp

$LicenseInfo:firstyear=2016&license=viewerlgpl$
Second Life Viewer Source Code
Copyright (C) 2022, Linden Research, Inc.

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


import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as nu
import time

# ideally the test run will supply its own limits
# but we set these defaults just in case
xlim = [-1.0, 1.0]
ylim = [-1.0, 1.0]
zlim = [0.0, 1.0]


# PUT IK TEST DATA HERE
#joint_configs = []
#initial_data = []
#solution_data = []
#xlim=[]
#ylim=[]
#zlim=[]

use_3d_mode = True


class Bone:
    def __init__(self, data):
        # data = (context, id, tip, bone)
        index, tip, bone = data
        self.index = str(index)
        self.x = (tip[0], tip[0] + bone[0])
        self.y = (tip[1], tip[1] + bone[1])
        self.z = (tip[2], tip[2] + bone[2])
        self.line = None

    def plot(self, ax):
        if use_3d_mode:
            self.line, = ax.plot3D(self.x, self.y, self.z)
        else:
            self.line, = ax.plot(self.x, self.y)

    def update(self, data):
        index, tip, bone = data
        self.x = (tip[0], tip[0] + bone[0])
        self.y = (tip[1], tip[1] + bone[1])
        self.z = (tip[2], tip[2] + bone[2])
        if self.line:
            self.line.set_data(self.x, self.y)
            if use_3d_mode:
                self.line.set_3d_properties(self.z)

def plot_frame(ax, frame):
    i = 0
    bones = {}
    for datum in frame:
        bone = Bone(datum)
        bone.plot(ax)
        bones[bone.index] = bone
    return bones

# create figure and axes
fig = plt.figure()
if use_3d_mode:
    ax = plt.axes(projection='3d') # 3D
else:
    ax = plt.axes() # 2D

ax.set(xlim=xlim)
ax.set(ylim=ylim)
ax.set(zlim=zlim)

# some text objects
data_x = xlim[0]
data_y = ylim[0]
data_z = zlim[0]
del_x = 0.08 * (xlim[1] - xlim[0])
del_y = 0.08 * (ylim[1] - ylim[0])
del_z = 0.08 * (zlim[1] - zlim[0])
loop_obj = ax.text(data_x + 3 * del_x, data_y + 3 * del_y, data_z + 3 * del_z, "loop=-1") 
context_obj = ax.text(data_x + 2 * del_x, data_y + 2 * del_y, data_z + 2 * del_z, "context=initial")
max_error_obj = ax.text(data_x + del_x, data_y + del_y, data_z + del_z, "max_error=???")

# create the bones
bone_index = -1
bones = plot_frame(ax, initial_data)

# configure the animation delay and update rate
interval = 800
initial_delay = int(4000 / interval)
data_index = - initial_delay

def animate(i):
    global bone_index
    global data_index
    actors = []
    if data_index > -1 and data_index < len(solution_data):
        while data_index < len(solution_data):
            data = solution_data[data_index]

            if data[0] == "context":
                context_obj.set_text(f"context={data[1]}")
            elif data[0] == "loop":
                loop_obj.set_text(f"loop={data[1]}")
            elif data[0] == "max_error":
                max_error_obj.set_text(f"max_error={data[1]}")
            else:
                bone_index = str(data[0])
                if bone_index in bones:
                    bone = bones[bone_index]
                    bone.update(data)
                data_index += 1
                break
            data_index += 1
    else:
        data_index += 1

    for bone in bones.values():
        if bone.index != bone_index:
            bone.line.set(linewidth=1.5)
        else:
            bone.line.set(linewidth=3.5)
        actors.append(bone.line)
    actors.append(context_obj)
    actors.append(loop_obj)
    actors.append(max_error_obj)

    return actors

# draw the targets
targets_x = []
targets_y = []
targets_z = []
for target in joint_configs:
    targets_x.append(target['p'][0])
    targets_y.append(target['p'][1])
    targets_z.append(target['p'][2])
if use_3d_mode:
    ax.scatter(targets_x, targets_y, targets_z)
else:
    ax.scatter(targets_x, targets_y)

# configure the animation
ani = animation.FuncAnimation(fig, animate, interval=interval, blit=True)

# show the figure
plt.show()


