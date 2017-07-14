list buttons = ["anim start", "anim stop", " ", "verbose on", "verbose off", " "];
string dialogInfo = "\nPlease make a choice.";
 
key ToucherID;
integer dialogChannel;
integer listenHandle;
integer commandChannel;
 
default
{
    state_entry()
    {
        dialogChannel = -1 - (integer)("0x" + llGetSubString( (string)llGetKey(), -7, -1) );
        commandChannel = -2001;
    }
 
    touch_start(integer num_detected)
    {
        ToucherID = llDetectedKey(0);
        llListenRemove(listenHandle);
        listenHandle = llListen(dialogChannel, "", ToucherID, "");
        llDialog(ToucherID, dialogInfo, buttons, dialogChannel);
        llSetTimerEvent(60.0); // Here we set a time limit for responses
    }
 
    listen(integer channel, string name, key id, string message)
    {
        if (message == "-")
        {
            llDialog(ToucherID, dialogInfo, buttons, dialogChannel);
            return;
        }
 
        llListenRemove(listenHandle);
        //  stop timer since the menu was clicked
        llSetTimerEvent(0);

        llOwnerSay("Sending message " + message + " on channel " + (string)commandChannel);
        llRegionSay(commandChannel, message);
    }
 
    timer()
    {
    //  stop timer
        llSetTimerEvent(0);
 
        llListenRemove(listenHandle);
        llWhisper(0, "Sorry. You snooze; you lose.");
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/axon/scripts/testing/lsl/axon_test_region_driver.lsl"
// End:
