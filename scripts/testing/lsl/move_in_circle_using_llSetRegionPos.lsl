integer listenHandle; 
integer verbose;
integer num_steps = 12;
float circle_time = 5.0;
integer circle_step;
vector circle_pos;
vector circle_center;
float circle_radius;

start_circle(vector center, float radius)
{
    vector currentPosition = llGetPos();
    circle_center = center;
    circle_radius = radius;
    circle_step = 0;
    llSetTimerEvent(circle_time/num_steps);
    llTargetOmega(<0.0, 0.0, 1.0>, TWO_PI/circle_time, 1.0);
}

stop_circle()
{
    llSetTimerEvent(0);
    llTargetOmega(<0.0, 0.0, 1.0>, TWO_PI/circle_time, 0.0);
    integer i;
    for (i=0; i<10; i++)
    {
        vector new_pos = circle_center;
        new_pos.x += llFrand(0.01);
        llSetRegionPos(new_pos);
        llSleep(0.1);
    }
}

next_circle()
{
    float rad = (circle_step * TWO_PI)/num_steps;
    float x = circle_center.x + llCos(rad)*circle_radius;
    float y = circle_center.y + llSin(rad)*circle_radius;
    float z = circle_center.z;
    llSetRegionPos(<x,y,z>);
    circle_step = (circle_step+1)%num_steps;
}

default
{
    state_entry()
    {
        //llSay(0, "Hello, Avatar!");
        listenHandle = llListen(-2001,"","","");  
        verbose = 0;
        circle_center = llGetPos();
    }

    listen(integer channel, string name, key id, string message)
    {
        //llOwnerSay("got message " + name + " " + (string) id + " " + message);
        list words = llParseString2List(message,[" "],[]);
        string command = llList2String(words,0);
        string option = llList2String(words,1);
        if (command=="anim")
        {
            if (option=="start")
            {
                start_circle(llGetPos(), 3.0);
            }
            else if (option=="stop")
            {
                stop_circle();
            }
        }
        if (command=="verbose")
        {
            if (option=="on")
            {
                verbose = 1;
            }
            else if (option=="off")
            {
                verbose = 0;
            }
        }
        if (command=="step")
        {
            llSetTimerEvent(0);
            next_circle();
        }
    }

    timer()
    {
        next_circle();
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/axon/scripts/testing/lsl/move_in_circle_using_llSetRegionPos.lsl"
// End:
