#!/usr/bin/python
"""\
@file   write_pilot_points.py
@author Callum Prentice
@date   2021-01-21
@brief  Generate and write out a pilot.txt file that can
        be used to control the path of an avatar via the 
        SL Viewer AutoPilot feature.  Put the generated 
        pilot.txt file next to the executable (for installed
        builds) or in indra/newview for dev builds.  Make
        sure pilot.xml is removed (pilot.txt is ignored
        if that is present) then restart Viewer.
        
        Format for file is:
        Number of entries N
        N entries, each of which is time (seconds), 0 (llagentpilot.h #39), global x, y, z

        We also create a PNG file that illustrates the 
        position of the points laid out on the region.
        This is purely for debugging purposes.
        
        If map tiles worked and were available, we could also
        display the single tile for a region which would
        be helpful.

$LicenseInfo:firstyear=2018&license=internal$
Copyright (c) 2018, Linden Research, Inc.
$/LicenseInfo$
"""
import math
from PIL import Image, ImageDraw

region_width = 256
region_height = 256

# STRAIGHT llagentpilot.h #39 - not clear what this does
action = 0  

# Global SL coords taken from Help > About
# Ideally we'd use local region coords since
# using global ones makes is specifc to a region
# Not clear the best way to get this base global
# coordinate - maybe a web service we can hit 
# and pass in region name?
region_x_global_pos = 335232
region_y_global_pos = 304768

# time (seconds) to take navigating through a set of way points
navigate_time_secs = 60.0

# Generate an Archimedes spiral that moves from the center
# of the region out to its edges.  The clever bit here it
# making the space between points all the same - otherwise
# the avatar movement at first is very shaky as the way
# points are very close together
def gen_arch_spiral(points):
    radius = region_width / 2.0 - 8.0
    arc_len = 10
    coils = 4
    theta_max = coils * 2 * math.pi
    step = radius / theta_max

    theta = arc_len / step
    while theta <= theta_max:
        x = image.width / 2 + math.cos(theta) * step * theta
        y = image.height / 2 + math.sin(theta) * step * theta

        theta += arc_len / (step * theta)

        points.append((x, y))

# Generatre a circle that loops around the outer
# extremity of the region
def gen_circle(points):
    radius = region_width / 2.0 - 8.0
    angle_max = 2 * math.pi

    angle = 0
    while angle < angle_max:
        x = image.width / 2 + math.cos(angle) * radius
        y = image.height / 2 + math.sin(angle) * radius

        angle += math.pi * 2 / 100

        points.append((x, y))


image = Image.new("RGB", (region_width, region_height), "white")
draw = ImageDraw.Draw(image)

for x in range(0, image.width, (int)(image.width / 16)):
    draw.line(((x, 0), (x, image.height)), fill=(224, 224, 224))

for y in range(0, image.height, (int)(image.height / 16)):
    draw.line(((0, y), (image.width, y)), fill=(224, 224, 224))

draw.line(((image.width / 2, 0), (image.width / 2, image.height)), fill=(224, 192, 192))
draw.line(((0, image.height / 2), (image.width, image.height / 2)), fill=(224, 192, 192))

points = []
gen_arch_spiral(points)
# gen_circle(points)

with open("pilot.txt", "w") as writer:
    writer.write(str(len(points)) + "\n")

    x_global_offset = region_width * int(region_x_global_pos / region_width)
    y_global_offset = region_height * int(region_y_global_pos / region_height)

    # hard coded for the test region - how do we not hard code this?
    z = 22.9854679107666015625

    for i, (x, y) in enumerate(points):
        ps = 4
        draw.ellipse((x - ps / 2, y - ps / 2, x + ps / 2, y + ps / 2), fill=(100, 200, 100))

        time = i * (navigate_time_secs / len(points))

        line = "%f %d %f %f %f\n" % (time, action, x + x_global_offset, y + y_global_offset, z)
        writer.write(line)

    image = image.resize((image.width * 2, image.height * 2), Image.ANTIALIAS)
    image.show()
    
    # more often than not, we don't need to save, just see
    #image.save("pilot_points.png", "PNG")
