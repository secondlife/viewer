Second Life - Linux Voice Support README
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

WHAT IS IT?
-=-=-=-=-=-

Linux Voice Support is a new feature in testing which allows users
of the Linux Second Life client to participate in voice-chat with other
residents and groups inside Second Life, with an appropriate
headset/microphone.

Linux Voice Support is currently EXPERIMENTAL and is known to still
have some compatibility issues.

REQUIREMENTS
-=-=-=-=-=-=

* A headset/microphone supported by your chosen version of Linux
* The ALSA sound system (you probably already have this -
  i.e. the alsa-base and alsa-utils packages on Ubuntu)

Success with Linux Voice support has been reported on the following
systems:
* Ubuntu 6.06 (Dapper) with Intel ICH5/CMI9761A+ audio chipset
* Ubuntu 6.06 (Dapper) with SigmaTel STAC2997 audio chipset
* Ubuntu 6.06 (Dapper) with Creative EMU10K1 audio chipset
* Ubuntu 7.04 (Feisty) with USB Plantronics headset
* Ubuntu 7.04 (Feisty) with Intel HDA audio chipset
* Fedora Core 6 with (unknown) audio chipset
* Ubuntu 8.04 (Hardy) with (unknown) audio chipset

KNOWN PROBLEMS
-=-=-=-=-=-=-=

* The 'Input Level' meter in the Voice Chat Device Settings dialog
  does not respond to audio input.

TROUBLESHOOTING
-=-=-=-=-=-=-=-

PROBLEM 1: I don't see a white dot over the head of my avatar or other
  Voice-using avatars.
SOLUTION:
a. Ensure that 'Enable voice chat' is enabled in the Voice Chat
  preferences window and that you are in a voice-enabled area (you
  will see a blue headphone icon in the SL menu-bar).
b. If the above does not help, exit Second Life and ensure that any
  remaining 'SLVoice' processes (as reported by 'ps', 'top' or similar)
  are killed.

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
a. Ensure that you have the 'Talk' button activated while you are trying to
  speak.
b. Ensure that your microphone jack is inserted into the correct socket of your
  sound card, where appropriate.
c. Use your system mixer-setting program or the 'alsamixer' program to ensure
  that microphone input is set as the active input source and is not muted.
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
