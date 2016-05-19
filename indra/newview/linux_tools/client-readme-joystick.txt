Second Life - Joystick & SpaceNavigator Support README
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

WHAT IS IT?
-=-=-=-=-=-

This feature allows the use of a joystick or other supported multi-axis
device for controlling your avatar and camera.

REQUIREMENTS
-=-=-=-=-=-=

* A joystick or other generic multi-axis input device supported by your chosen
  version of Linux

- OR -

* A SpaceNavigator device (additional configuration may be required, see below)

Success has been reported on the following systems so far:
* Ubuntu 7.04 (Feisty) with a generic USB joystick
* Ubuntu 7.04 (Feisty) with a USB 3DConnexion SpaceNavigator
* Ubuntu 6.06 (Dapper) with a generic USB joystick
* Ubuntu 6.06 (Dapper) with a USB 3DConnexion SpaceNavigator

CONFIGURATION
-=-=-=-=-=-=-

SPACE NAVIGATOR: *Important* - do not install the Linux SpaceNavigator
drivers from the disk included with the device - these are problematic.
Some distributions of Linux (such as Ubuntu, Gentoo and Mandriva) will
need some system configuration to make the SpaceNavigator usable by
applications such as the Second Life Viewer, as follows:

* Mandriva Linux Configuration:
  You need to add two new files to your system.  This only needs to be
  done once.  These files are available at the 'SpaceNavigator support with
  udev and Linux input framework' section of
  <http://www.aaue.dk/~janoc/index.php?n=Personal.Downloads>

* Ubuntu or Gentoo Linux Configuration:
  For a quick start, you can simply paste the following line into a terminal
  before plugging in your SpaceNavigator - this only needs to be done once:
  sudo bash -c 'echo KERNEL==\"event[0-9]*\", SYSFS{idVendor}==\"046d\", SYSFS{idProduct}==\"c626\", SYMLINK+=\"input/spacenavigator\", GROUP=\"plugdev\", MODE=\"664\" > /etc/udev/rules.d/91-spacenavigator-LL.rules ; echo "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?><deviceinfo version=\"0.2\"><device><match key=\"info.product\" contains=\"3Dconnexion SpaceNavigator\"><merge key=\"input.x11_driver\" type=\"string\"></merge></match></device></deviceinfo>" > /etc/hal/fdi/policy/3Dconnexion_SpaceNavigator_LL.fdi'

For more comprehensive Linux SpaceNavigator configuration information please
see the section 'Installing SpaceNavigator without the official driver' here:
<http://www.aaue.dk/~janoc/index.php?n=Personal.3DConnexionSpaceNavigatorSupport>

JOYSTICKS: These should be automatically detected and configured on all
modern distributions of Linux.

ALL: Your joystick or SpaceNavigator should be plugged-in before you start the
Second Life Viewer, so that it may be detected.  If you have multiple input
devices attached, only the first detected SpaceNavigator or joystick device
will be available.

Once your system recognises your joystick or SpaceNavigator correctly, you
can go into the Second Life Viewer's Preferences dialog, click the 'Input &
Camera' tab, and click the 'Joystick Setup' button.  From here you may enable
and disable joystick support and change some configuration settings such as
sensitivity.  SpaceNavigator users are recommended to click the
'SpaceNavigator Defaults' button.

KNOWN PROBLEMS
-=-=-=-=-=-=-=

* If your chosen version of Linux treats your joystick/SpaceNavigator as
if it were a mouse when you plug it in (i.e. it is automatically used to control
your desktop cursor), then the SL Viewer may detect this device *but* will be
unable to use it properly.

FURTHER PROBLEMS?
-=-=-=-=-=-=-=-=-

Please report further issues to the public Second Life issue-tracker
at <http://jira.secondlife.com/> (please note, however, that this is not
a support forum).
