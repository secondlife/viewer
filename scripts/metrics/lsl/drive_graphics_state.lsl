integer commandChannel = -777; // for sending commands to listeners
integer backChannel = -666; // for listeners responding to the driver
integer backHandle;
key ToucherID;

list buttons = ["test start", "test stop", " ", "verbose on", "verbose off", " "];
string dialogInfo = "\nPlease make a choice.";
integer dialogChannel = -888; // for dialog
integer dialogHandle; // listen handle for dialog

list properties = ["alpha", "glow", "shiny", "point_light",
                   "materials", "specmap", "normalmap", "bump", 
                   "texture_res_128", "texture_res_1024"];
integer propertyIndex = -1;
string prop;

integer acksStarted;
integer acksDone;

integer testDuration = 30; // 30
integer maxAckDoneWaitDuration = 120; // 120
integer maxAckStartWaitDuration = 10; // 10


default
{
    state_entry()
    {
        llOwnerSay("driver is starting");
        propertyIndex = -1;
    }

    listen(integer channel, string name, key id, string message)
    {
        if (channel == dialogChannel)
        {
            if (message == "-")
            {
                llDialog(ToucherID, dialogInfo, buttons, dialogChannel);
                return;
            }
            llListenRemove(dialogHandle);

            if (message == "test start")
            {
                state nextProperty;
            }
            else if ((message == "verbose on") || (message == "verbose off"))
            {
                llOwnerSay("Sending command: " + message);
                llSay(commandChannel,message);
            }
            else
            {
                llOwnerSay("don't know how to handle dialog choice: " + message);
            }
        }
    }
 
    touch_end(integer num_detected)
    {
        ToucherID = llDetectedKey(0);
        llListenRemove(dialogHandle);
        dialogHandle = llListen(dialogChannel, "", ToucherID, "");
        llDialog(ToucherID, dialogInfo, buttons, dialogChannel);
    }
 
}

state noMoreProperties
{
    state_entry()
    {
        llOwnerSay("SUCCESS! done all properties");
        llSetText("DONE!",<0,1,0>,1);
        state default;
    }
}

state waitForPropDone
{
    state_entry()
    {
        backHandle = llListen(backChannel, "", "", "");
        llSetTimerEvent(maxAckDoneWaitDuration);

        if (acksDone >= acksStarted)
        {
            llOwnerSay("All started acks are done " + (string) acksStarted + "/" + (string) acksDone);
            state nextProperty;
        }
    }

    timer()
    {
        llOwnerSay("Done waiting for acks done");
        llOwnerSay("Remaining started/done: " + (string) acksStarted + "/" + (string) acksDone);
        llOwnerSay("ERROR! Out of sync, quitting");
        llSetText("ERROR!",<1,0,0>,1);
        state default;
    }
    
    listen(integer channel, string name, key id, string message)
    {
        if (message=="started")
        {
            acksStarted += 1;
        }
        if (message=="done")
        {
            acksDone += 1;
        }
        if (acksDone >= acksStarted)
        {
            llOwnerSay("prop " + prop + " done; all started acks completed " + (string) acksStarted + "/" + (string) acksDone);
            state nextProperty;
        }
    }
}

state nextProperty
{
    state_entry()
    {
        propertyIndex += 1;
        integer length = llGetListLength(properties);
        if (propertyIndex >= length)
        {
            state noMoreProperties;
        }
        prop = llList2String(properties, propertyIndex);
        llSetText("Running [" + prop + "]...",<0,0,1>,1);
        llOwnerSay("requesting start_prop " + prop);
        backHandle = llListen(backChannel, "", "", "");
        llSetTimerEvent(maxAckStartWaitDuration);
        llSay(commandChannel,"start_prop " + prop + " " + (string) testDuration );
        acksStarted = 0;
        acksDone = 0;
    }

    state_exit()
    {
        llSetTimerEvent(0);
    }

    timer()
    {
        state waitForPropDone;
    }
    
    listen(integer channel, string name, key id, string message)
    {
        if (message=="started")
        {
            acksStarted += 1;
        }
        if (message=="done")
        {
            acksDone += 1;
        }
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/arctan/scripts/metrics/lsl/drive_graphics_state.lsl"
// End:
