integer command_channel;
integer backchat_channel;
integer dialog_channel;

integer listen_handle;

key toucher_id;
list buttons = ["start", "stop"];
string dialog_info = "\nPlease make a choice.";

stop_tests()
{
    llOwnerSay("stopping tests");
}

start_tests()
{
    llOwnerSay("starting tests");

    float test_time = 30.0;
    float pause_time = 20.0;
    list properties = ["alpha", "glow", "shiny", "point_light",
                       "materials", "specmap", "normalmap", "bump", 
                       "texture_res_128", "texture_res_1024"];
    integer length = llGetListLength(properties);
    integer index = 0;
    while (index < length)
    {
        string prop = llList2String(properties, index);
        ++index;
            
        llOwnerSay("Starting " + prop);
        llSay(command_channel, prop + " start");
        llSleep(test_time);
        llOwnerSay("Done " + prop);
        llSay(command_channel,prop + " stop");
        llSleep(pause_time);
    }
}

handle_button(string button_message)
{
    if (button_message == "start")
    {
        start_tests();
    }
    if (button_message == "stop")
    {
        stop_tests();
    }
}

default
{
    state_entry()
    {
        // arbitrary
        command_channel = -777;
        dialog_channel = -555;
        backchat_channel = -666;
        
        llOwnerSay("driver is starting");
    }

    touch_start(integer total_number)
    {
        toucher_id = llDetectedKey(0);
        llListenRemove(listen_handle);
        listen_handle = llListen(dialog_channel, "", toucher_id, "");
        llDialog(toucher_id, dialog_info, buttons, dialog_channel);
    }

    listen(integer channel, string name, key id, string button_message)
    {
        if (button_message == "-")
        {
            llDialog(toucher_id, dialog_info, buttons, dialog_channel);
            return;
        }
 
        llListenRemove(listen_handle);

        handle_button(button_message);
    }
 
}

// Local Variables:
// shadow-file-name: "$SW_HOME/arctan/scripts/metrics/lsl/drive_graphics_state.lsl"
// End:
