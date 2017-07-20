integer listenHandle; 
integer verbose;
integer num_steps = 50;
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
}

stop_circle()
{
    llSetTimerEvent(0);
    llSetRegionPos(circle_center);
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

circle_path(vector center, float radius)
{
    integer i;
    integer num_steps = 50;
    float circle_time = 5.0; // seconds
    for (i=0; i<num_steps; ++i)
    {
        float rad = (i * TWO_PI)/num_steps;
        float x = center.x + llCos(rad)*radius;
        float y = center.y + llSin(rad)*radius;
        float z = center.z;
        llSetRegionPos(<x,y,z>);
        llSleep(circle_time/num_steps);
    }
}

default
{
    state_entry()
    {
        llSay(0, "Hello, Avatar!");
        listenHandle = llListen(-2001,"","","");  
        verbose = 0;
    }

    listen(integer channel, string name, key id, string message)
    {
        llOwnerSay("got message " + name + " " + (string) id + " " + message);
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
    }

    timer()
    {
        next_circle();
    }
}

// Local Variables:
// shadow-file-name: "$SW_HOME/axon/scripts/testing/lsl/move_in_circle_using_llSetRegionPos.lsl"
// End:
