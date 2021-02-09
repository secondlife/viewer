#!/usr/bin/python
"""\
@file   generate_texture_fetch_textures.py
@author Callum Prentice
@date   2021-01-21
@brief  Generate and write out a set of test textures
        that can be uploaded to SL (typically using
        Bulk Upload) and uses to texture the region
        we are using to test and gather metrics for the
        upcoming texture fetch work.  
        
        Each texture is of course different and is
        comprised of a checkboard of 2 random colors
        and a label for the name/id and dimensions.
        
        Evenutally, it would be great to automate
        this even more and write LSL scripts to 
        generate these textures (not possible right
        now) and then apply to to sets of created and
        positioned prims. 
        
        For the momeent, create the set of textures,
        upload to SL, create your set of prims and 
        assign one of the uploaded textures to each.

$LicenseInfo:firstyear=2018&license=internal$
Copyright (c) 2018, Linden Research, Inc.
$/LicenseInfo$
"""
import os
import sys
import argparse
import math
from random import seed
from random import randint
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw

parser = argparse.ArgumentParser(
    description="Generates random textures and saves them as JPEG files."
)

parser.add_argument(
    "-n --num_textures",
    nargs=1,
    type=int,
    required=True,
    help="Number of textures to generate",
    dest="num_textures",
)

parser.add_argument(
    "-min --min_size",
    nargs=1,
    type=int,
    required=True,
    help="Minimum dimensions of the texture",
    dest="min_size",
)

parser.add_argument(
    "-max --max_size",
    nargs=1,
    type=int,
    required=True,
    help="Maximum dimensions of the texture",
    dest="max_size",
)

parser.add_argument(
    "-f --first_num",
    nargs=1,
    type=int,
    required=True,
    help="First number to use in texture filename",
    dest="first_num",
)

args = parser.parse_args()

saturation = 90
luminance = 50

# calculate how much padding we need for this number range
max_index = args.num_textures[0] + args.first_num[0]
num_padding_digits = int(round(math.log10(max_index) + 0.5, 0))
num_padding_str = "0%d" % num_padding_digits

# Overkill perhaps but use a decent font so we can resize it
# vs the one built into PIL
def get_font_path():
    macos_font_path = "/Library/Fonts/Arial Unicode.ttf"
    if os.path.isfile(macos_font_path) == True:
        return macos_font_path
    windows_font_path = "/Windows/fonts/arial.ttf"
    if os.path.isfile(windows_font_path) == True:
        return windows_font_path
    default_path = ""
    return default_path


# Write a line of text that is scaled to fit horizontally.  We allow
# the writing of a "Small" version - consider it a second level heading
def write_line(img, line, font_path, max_height, y, col, small_font=False):
    padding = 32

    draw = ImageDraw.Draw(img)

    target_w = img.size[0] - padding
    target_h = max_height

    if small_font == True:
        target_w = (img.size[0] - padding) / 2.0
        target_h = max_height
    max_font_size = 160
    min_font_size = 8
    for fs in range(max_font_size, min_font_size, -2):
        font = ImageFont.truetype(font_path, fs)
        text_w, text_h = draw.textsize(line, font=font)
        if text_w < target_w and text_h < target_h:
            break
    draw.text(
        ((img.size[0] - text_w) / 2, y - text_h / 2), line, col, font=font
    )


# actually write out the textures - we might consider making all
# these values command line parameters one day.
for t in range(args.num_textures[0]):
    texture_width = randint(args.min_size[0], args.max_size[0])
    texture_height = randint(args.min_size[0], args.max_size[0])

    # override for now - looks better on a cube when width == height
    texture_height = texture_width

    # todo - make a parameter??
    num_squares_x = randint(4, 24)
    num_squares_y = int(texture_height * num_squares_x / texture_width)

    image = Image.new("RGB", (texture_width, texture_height), "white")
    draw = ImageDraw.Draw(image)

    # use the same saturation and luminence - just change the hue of
    # each color. TODO: find a way to make the colors more complimentary
    # vs just both random - too often get very similar colors
    color1 = "hsl(%d, %d%%, %d%%)" % (randint(0, 120), saturation, luminance)
    color2 = "hsl(%d, %d%%, %d%%)" % (randint(135, 255), saturation, luminance)

    for y in range(num_squares_y):
        for x in range(num_squares_x):
            draw.rectangle(
                (
                    (
                        texture_width * x / num_squares_x,
                        texture_height * y / num_squares_y,
                    ),
                    (
                        (texture_width * (x + 1)) / num_squares_x,
                        (texture_height * (y + 1)) / num_squares_y,
                    ),
                ),
                color1 if (x + y) % 2 == 0 else color2,
            )

    image_number = args.first_num[0] + t
    texture_num_str = format(image_number, num_padding_str)
    line = "Random Texture # %s" % (texture_num_str)
    write_line(
        image,
        line,
        get_font_path(),
        texture_height / 2,
        texture_height * 1 / 4,
        "black",
        False,
    )

    line = "%d x %d" % (texture_width, texture_height)
    write_line(
        image,
        line,
        get_font_path(),
        texture_height / 2,
        texture_height * 3 / 4,
        "black",
        True,
    )

    # image.show()
    filename = "generated_texture_%s.jpg" % (texture_num_str)
    image.save(filename, "JPEG")
    print "Writing texture: ", filename
