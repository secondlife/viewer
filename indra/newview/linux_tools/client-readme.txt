Second Life - Linux Beta README
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

This document contains information about the Second Life Linux
client, and isn't meant to serve as an introduction to Second
Life itself - please see <http://www.secondlife.com/whatis/>.

1. Introduction
2. System Requirements
3. Installing & Running
4. Known Issues
5. Troubleshooting
   5.1. 'Error creating window.'
   5.2. System hangs
   5.3. Blank window after minimizing it
   5.4. Audio
   5.5. 'Alt' key for camera controls doesn't work
   5.6. In-world streaming movie, music and Flash playback
6. Advanced Troubleshooting
   6.1. Audio
   6.2. OpenGL
7. Obtaining and working with the client source code
8. Getting more help, and reporting problems


1. INTRODUCTION
-=-=-=-=-=-=-=-

Hi!  This is a BETA release of the Second Life client for Linux.
The 'beta' status means that although we're still smoothing-out a few rough
edges, this version of the client is functionally complete and should
work quite well 'out of the box' for accessing Second Life.

We encourage you to try it out and let us know of its compatibility
with your system.  Be aware that although this is a 'beta' client, it connects
to the main Second Life world and changes you make there are permanent.

You will have either obtained this client from secondlife.com (the official
site) or from a third-party packager; if you encounter any problems then
we recommend trying the latest official builds from secondlife.com which are
updated often.

Please enjoy!


2. SYSTEM REQUIREMENTS
-=-=-=-=-=-=-=-=-=-=-=

Minimum requirements:
    * Internet Connection: Cable or DSL
    * Computer Processor: 800MHz Pentium III or Athlon or better
      (recommended: 1.5GHz or more)
    * Computer Memory: 512MB (recommended: 768MB or more)
    * Linux Operating System: A reasonably modern 32-bit Linux environment
          is required.  If you are running a 64-bit Linux distribution then
          you will need its 32-bit compatibility environment installed, but
          this configuration is not currently supported.
    * PulseAudio or ALSA Linux system sound software.  A recent PulseAudio
      is the recommended configuration; see README-linux-voice.txt for more
      information.
    * Video/Graphics Card:
          o nVidia GeForce 2, GeForce 4mx, or better (recommend one of the
            following: 6700, 6800, 7600, 7800, 7900, 8400, 8500, 8600,
            8800, Go 7400, Go 7600, Go 7800, Go 7900, +)
          o OR ATI Radeon 8500, 9250, or better
          (nVidia cards are recommended for the Linux client)

      **NOTE**: Second Life absolutely requires you to have recent, correctly-
      configured OpenGL 3D drivers for your hardware - the graphics drivers
      that came with your operating system may not be good enough!  See the
      TROUBLESHOOTING section if you encounter problems starting Second Life.

For a more comfortable experience, the RECOMMENDED hardware for the Second
Life Linux client is very similar to that for Windows, as detailed at:
<https://secondlife.com/corporate/sysreqs.php>


3. INSTALLING & RUNNING
-=-=-=-=-=-=-=-=-=-=-=-

The Second Life Linux client can entirely run from the directory you have
unpacked it into - no installation step is required.  If you wish to
perform a separate installation step anyway, you may run './install.sh'

Run ./secondlife from the installation directory to start Second Life.

For in-world MOVIE and MUSIC PLAYBACK, you will need (32-bit) GStreamer 0.10
installed on your system.  This is optional - it is not required for general
client functionality.  If you have GStreamer 0.10 installed, the selection of
in-world movies you can successfully play will depend on the GStreamer
plugins you have; if you cannot play a certain in-world movie then you are
probably missing the appropriate GStreamer plugin on your system - you may
be able to install it (see TROUBLESHOOTING).

User data is stored in the hidden directory ~/.secondlife by default; you may
override this location with the SECONDLIFE_USER_DIR environment variable if
you wish.


4. KNOWN ISSUES
-=-=-=-=-=-=-=-

* No significant known issues at this time.


5. TROUBLESHOOTING
-=-=-=-=-=-=-=-=-=

The client prints a lot of diagnostic information to the console it was
run from.  Most of this is also replicated in ~/.secondlife/logs/SecondLife.log
- this is helpful to read when troubleshooting, especially 'WARNING' and
'ERROR' lines.

VOICE PROBLEMS?  See the separate README-linux-voice.txt file for Voice
  troubleshooting information.

SPACENAVIGATOR OR JOYSTICK PROBLEMS?  See the separate
  README-linux-joystick.txt file for configuration information.

PROBLEM 1:- Second Life fails to start up, with a warning on the console like:
   'Error creating window.' or
   'Unable to create window, be sure screen is set at 32-bit color' or
   'SDL: Couldn't find matching GLX visual.'
SOLUTION:- Usually this indicates that your graphics card does not meet
   the minimum requirements, or that your system's OpenGL 3D graphics driver is
   not updated and configured correctly.  If you believe that your graphics
   card DOES meet the minimum requirements then you likely need to install the
   official so-called 'non-free' nVidia or ATI (fglrx) graphics drivers; we
   suggest one of the following options:
 * Consult your Linux distribution's documentation for installing these
   official drivers.  For example, Ubuntu provides documentation here:
   <https://help.ubuntu.com/community/BinaryDriverHowto>
 * If your distribution does not make it easy, then you can download the
   required Linux drivers straight from your graphics card manufacturer:
   - nVidia cards: <http://www.nvidia.com/object/unix.html>
   - ATI cards: <http://ati.amd.com/support/driver.html>

PROBLEM 2:- My whole system seems to hang when running Second Life.
SOLUTION:- This is typically a hardware/driver issue.  The first thing to
   do is to check that you have the most recent official drivers for your
   graphics card (see PROBLEM 1).
SOLUTION:- Some residents with ATI cards have reported that running
   'sudo aticonfig --locked-userpages=off' before running Second Life solves
   their stability issues.
SOLUTION:- As a last resort, you can disable most of Second Life's advanced
   graphics features by editing the 'secondlife' script and removing the '#'
   from the line which reads '#export LL_GL_NOEXT=x'

PROBLEM 3:- After I minimize the Second Life window, it's just blank when
   it comes back.
SOLUTION:- Some Linux desktop 'Visual Effects' features are incompatible
   with Second Life.  One reported solution is to use your desktop
   configuration program to disable such effects.  For example, on Ubuntu 7.10,
   use the desktop toolbar menu to select System -> Preferences -> Appearance,
   then change 'Visual Effects' to 'None'.

PROBLEM 4:- Music and sound effects are silent or very stuttery.
SOLUTION:- The most common solution is to ensure that you have the 'esd'
   program (part of the 'esound' package) installed and running before you
   start Second Life.  Users of Ubuntu (and some other) Linux systems can
   simply run the following to install and configure 'esound':
     sudo apt-get install esound
  For others, simply running 'esd&' from a command-line should get it running.

PROBLEM 5:- Using the 'Alt' key to control the camera doesn't work or just
   moves the Second Life window.
SOLUTION:- Some window managers eat the Alt key for their own purposes; you
   can configure your window manager to use a different key instead (for
   example, the 'Windows' key!) which will allow the Alt key to function
   properly with mouse actions in Second Life and other applications.

PROBLEM 6:- In-world movie, music, or Flash playback doesn't work for me.
SOLUTION:- You need to have a working installation of GStreamer 0.10; this
   is usually an optional package for most versions of Linux.  If you have
   installed GStreamer 0.10 and you can play some music/movies but not others
   then you need to install a wider selection of GStreamer plugins, either
   from your vendor or an appropriate third party.
   For Flash playback, you need to have Flash 10 installed for your normal
   web browser (for example, Firefox).  PulseAudio is required for Flash
   volume control / muting to fully function inside Second Life.


6. ADVANCED TROUBLESHOOTING
-=-=-=-=-=-=-=-=-=-=-=-=-=-

The 'secondlife' script which launches Second Life contains some
configuration options for advanced troubleshooters.

* AUDIO - Edit the 'secondlife' script and you will see these audio
  options: LL_BAD_OPENAL_DRIVER, LL_BAD_FMOD_ESD, LL_BAD_FMOD_OSS, and
  LL_BAD_FMOD_ALSA.  Second Life tries to use OpenAL, ESD, OSS, then ALSA
  audio drivers in this order; you may uncomment the corresponding LL_BAD_*
  option to skip an audio driver which you believe may be causing you trouble.

* OPENGL - For advanced troubleshooters, the LL_GL_BLACKLIST option lets
  you disable specific GL extensions, each of which is represented by a
  letter ("a"-"o").  If you can narrow down a stability problem on your system
  to just one or two GL extensions then please post details of your hardware
  (and drivers) to the Linux Client Testers forum (see link below) along
  with the minimal LL_GL_BLACKLIST which solves your problems.  This will help
  us to improve stability for your hardware while minimally impacting
  performance.
  LL_GL_BASICEXT and LL_GL_NOEXT should be commented-out for this to be useful.


7. OBTAINING AND WORKING WITH THE CLIENT SOURCE CODE
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

We're pleased to have released the Second Life client's source code under
an Open Source license compatible with the 'GPL'.  To get involved with client
development, please see:
<http://wiki.secondlife.com/wiki/Open_Source_Portal>


8. GETTING MORE HELP AND REPORTING PROBLEMS
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

For general help and support with Second Life:
<http://secondlife.com/community/support.php>

For problems and discussion concerning unofficial (not secondlife.com)
releases, please contact your packager or the SLDev mailing list:
<https://lists.secondlife.com/cgi-bin/mailman/listinfo/sldev>

In-world help: Please use the 'Help' menu in the client for various
non-Linux-specific Second Life help options.

In-world discussion: There is a 'Linux Client Users' group
inside Second Life which is free to join.  You can find it by pressing
the 'Search' button at the bottom of the window and then selecting the
'Groups' tab and searching for 'Linux'.  This group is useful for discussing
Linux issues with fellow Linux client users who are online.

The Second Life Issue Tracker:
<http://jira.secondlife.com/>
This is the right place for finding known issues and reporting new
bugs in all Second Life releases if you find that the Troubleshooting
section in this file hasn't helped (please note, however, that this is
not a support forum).

Linux Client Testers forum:
<http://forums.secondlife.com/forumdisplay.php?forumid=263>
This is a forum where Linux Client users can help each other out and
discuss the latest updates.
