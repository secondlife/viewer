Second Life - Linux Voice Support README
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

WHAT IS IT?
-=-=-=-=-=-

Linux Voice Support is a feature in testing which allows users of the Linux
Second Life client to participate in voice-chat with other residents and
groups inside Second Life, with an appropriate headset/microphone.

REQUIREMENTS
-=-=-=-=-=-=

* A headset/microphone supported by your chosen version of Linux
* At this time, the PulseAudio audio system is recommended; this software
  is already part of most modern (2009+) Linux desktop systems.  Alternatively,
  the ALSA audio system may be used on systems installed from around
  2007 onwards (again this is likely already installed on your system).

KNOWN PROBLEMS
-=-=-=-=-=-=-=

* Compatibility with old ALSA-based audio systems (such as Ubuntu Dapper
  from 2006) is poor.

TROUBLESHOOTING
-=-=-=-=-=-=-=-

PROBLEM 1: I don't see a white dot over the head of my avatar or other
  Voice-using avatars.
SOLUTION:
a. Ensure that 'Enable voice' is enabled in the 'Sound' section of the
  Preferences window, and that you are in a voice-enabled area.
b. If the above does not help, exit Second Life and ensure that any
  remaining 'SLVoice' processes (as reported by 'ps', 'top' or similar)
  are killed before restarting.

PROBLEM 2: I have a white dot over my head but I never see (or hear!) anyone
  except myself listed in the Active Speakers dialog when I'm sure that other
  residents nearby are active Voice users.
SOLUTION: This is an incompatibility between the Voice support and your
  system's audio (ALSA) driver version/configuration.
a. Back-up and remove your ~/.asoundrc file, re-test.
b. Check for updates to your kernel, kernel modules and ALSA-related
  packages using your Linux distribution's package-manager - install these,
  reboot and re-test.
c. Update to the latest version of ALSA manually.  For a guide, see the
  'Update to the Latest Version of ALSA' section of this page:
  <https://help.ubuntu.com/community/HdaIntelSoundHowto> or the official
  documentation on the ALSA site: <http://www.alsa-project.org/> - reboot
  and re-test.

PROBLEM 3: I can hear other people, but they cannot hear me.
SOLUTION:
a. Ensure that you have the 'Speak' button (at the bottom of the Second Life
   window) activated while you are trying to speak.
b. Ensure that your microphone jack is inserted into the correct socket of your
  sound card, where appropriate.
c. Use your system mixer-setting program (such as the PulseAudio 'volume
  control' applet or the ALSA 'alsamixer' program) to ensure that microphone
  input is set as the active input source and is not muted.
d. Verify that audio input works in other applications, i.e. Audacity

PROBLEM 4: Other people just hear bursts of loud noise when I speak.
SOLUTION:
a. Use your system mixer-setting program or the 'alsamixer' program to ensure
  that microphone Gain/Boost is not set too high.

FURTHER PROBLEMS?
-=-=-=-=-=-=-=-=-

Please report further issues to the public Second Life issue-tracker
at <http://jira.secondlife.com/> (please note, however, that this is not
a support forum).
