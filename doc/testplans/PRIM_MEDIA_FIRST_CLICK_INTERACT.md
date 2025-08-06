# Test plan for PRIM_MEDIA_FIRST_CLICK_INTERACT

## Requirements

- At least two accounts
- At least one group
- Land under your control

## Feature Brief

Historically media-on-a-prim (MOAP) in Second Life has been bound to a focus system which blocks mouse click/hover events, this feature creates exceptions to this focus system for a configurable set of objects to meet user preference.

## Testing

The following scripts and test cases cover each individual operational mode of the feature; in practice these modes can be combined by advanced users in any configuration they desire from debug settings. Even though the intended use case combines multiple modes, individual modes can be  tested for functionality when tested as described below.

If testing an arbitrary combination of operational modes beyond what the GUI offers is desired, the parameters of the bitfield for calculation are located in lltoolpie.h under the MediaFirstClickTypes enum. As of writing there exists a total of ~127 possible unique/valid combinations, which is why testing each mode individually is considered the most efficient for a full functionality test.

### Scripts

#### Script A

This script creates a media surface that is eligible for media first-click interact. Depending on test conditions, this will exhibit new behavior.

```lsl
default {
    state_entry() {
        llSetLinkMedia( LINK_THIS, 0, [
            PRIM_MEDIA_CURRENT_URL, "http://lecs-viewer-web-components.s3.amazonaws.com/v3.0/agni/avatars.html",
            PRIM_MEDIA_HOME_URL, "http://lecs-viewer-web-components.s3.amazonaws.com/v3.0/agni/avatars.html",
            PRIM_MEDIA_FIRST_CLICK_INTERACT, TRUE,
            PRIM_MEDIA_AUTO_PLAY, TRUE,
            PRIM_MEDIA_CONTROLS, PRIM_MEDIA_CONTROLS_MINI
        ] );
    }
}

```

#### Script B

This script creates a media surface that is NOT eligible for media first-click interact. In all but one test case, this will behave the same way.

```lsl
default {
    state_entry() {
        llSetLinkMedia( LINK_THIS, 0, [
            PRIM_MEDIA_CURRENT_URL, "http://lecs-viewer-web-components.s3.amazonaws.com/v3.0/agni/avatars.html",
            PRIM_MEDIA_HOME_URL, "http://lecs-viewer-web-components.s3.amazonaws.com/v3.0/agni/avatars.html",
            PRIM_MEDIA_FIRST_CLICK_INTERACT, FALSE,
            PRIM_MEDIA_AUTO_PLAY, TRUE,
            PRIM_MEDIA_CONTROLS, PRIM_MEDIA_CONTROLS_MINI
        ] );
    }
}

```

## Standard testing procedure

You will be asked to enable media faces on multiple cubes, make sure that the webpage loads on each, and interact with them in the following ways.

1. Enable media for the cube, and verify that it displays a webpage.
2. Click on the terrain to clear any focus.
3. Hover your mouse over UI elements of the webpage, and **observe** if they highlight/react to the mouse cursor.
4. If hover events are not registered, clicking on the webpage and then **observe** if they begin reacting to hover events.
5. Clicking on the terrain to clear any focus once again.
6. Clicking on a UI element of the webpage and **observe** if it reacts to the first click, or requires a second click. *(Maximum of 2 clicks per attempt)*

These steps will be repeated for one or more pairs of cubes per test case to ensure that media first click interact is functioning within expectations. Unless otherwise mentioned for a specific test case, you simply need only be in the same region as the cubes to test with them.

## Test cases

All test cases begin with at least two cubes rezzed, one containing Script A henceforth referred to as Cube A and one with Script B referred to as Cube B. The steps of some test cases may impact the condition of the cubes, so keeping a spare set rezzed or in inventory to rapidly duplicate should improve efficiency if testing cases in series.

### Case 1 (MEDIA_FIRST_CLICK_NONE)

Ensure that debug setting `MediaFirstClickInteract` is set to `0`

Starting with Cube A and Cube B, perform the testing procedure on each.

**Expected observations:** Both webpages do not react to hover events until clicked, both webpages do not react to clicks until clicked once to establish focus

### Case 2 (MEDIA_FIRST_CLICK_HUD)

Ensure that debug setting `MediaFirstClickInteract` is set to `1`

Starting with Cube A and Cube B, attach them both to your HUD and perform the testing procedure on each. You may need to rotate or scale the cubes to fit on your screen before beginning testing. You may attach both at the same time, or only one at a time.

**Expected observations:** The webpage on Cube A will react to mouse cursor hover events and clicks without needing a focus click, but the webpage on Cube B will not.

### Case 3 (MEDIA_FIRST_CLICK_OWN)

Ensure that debug setting `MediaFirstClickInteract` is set to `2`

This test case requires two pairs of cubes, and the second pair must not be owned by your testing account. What owns them is not important, it can be a group or your second testing account.

Perform the testing procedure on both sets of cubes.

**Expected observations:** The webpage on Cube A will react to mouse cursor hover events and clicks without needing a focus click, but the webpage on Cube B will not. The other pair of cubes will react the same as your Cube B.

### Case 4 (MEDIA_FIRST_CLICK_GROUP)

Ensure that debug setting `MediaFirstClickInteract` is set to `4`

This test case requires two cubes, and the second cube must be deeded or set to a group that your testing account is a member of. As long as the second set of cubes is set to a group that your test account is a member of, the avatar that owns them does not matter.

Perform the testing procedure on both sets of cubes.

**Expected observations:** The cube owned by your primary account will not react to mouse cursor hover events and clicks without needing a focus click. The cube set to group will react to mouse cursor hover events and clicks without needing a focus click.

### Case 5 (MEDIA_FIRST_CLICK_FRIEND)

Ensure that debug setting `MediaFirstClickInteract` is set to `8`

This test case requires three sets of cubes, one owned by you, one owned by another avatar on your friend list, and a third set owned by an avatar that is not on your friend list, or deeded to group. You can optionally use two sets of cubes, and dissolve friendship with your second account to test non-friend cubes.

Perform the testing procedure on all cubes

**Expected observations:** Cube A owned by a friended avatar will react to mouse cursor hover events and clicks without needing a focus click. All other cubes will not.

### Case 6 (MEDIA_FIRST_CLICK_LAND)

Ensure that debug setting `MediaFirstClickInteract` is set to `16`

This is the most tricky test case due to the multiple combinations that this operational mode considers valid. You will need multiple cubes, and can omit Cube B for brevity unless running a full test pass. This is probably most efficiently tested from your second account, using your first account to adjust the test parameters to fit other sub-cases.

Note: This requires the avatar that is performing the tests to physically be in the same parcel as the test cube(s). If you are standing outside of the parcel the media cubes are in, they will never react to mouse cursor hover events and clicks without needing a focus click under this operational mode.

1. Place down a set of cubes owned by the same avatar as the land
    - The second account should see Cube A react to mouse cursor hover events and clicks without needing a focus click
    - Cube B if tested, will not react in all further sub-cases and will not be mentioned further.

2. Adjust the conditions of the cubes and parcel such that they are owned by another avatar, but have the same group as the land set
    - The second account should see Cube A react to mouse cursor hover events and clicks without needing a focus click

3. Adjust the conditions of the cubes and parcel such that they are deeded to the same group that the parcel is deeded to
    - The second account should see Cube A react to mouse cursor hover events and clicks without needing a focus click

4. Adjust the conditions of the cubes and parcel such that the parcel and cubes do not share an owner, or a group
    - The second account should see Cube A NOT react to mouse cursor hover events until clicked, and clicks WILL need a focus click before registering.

### Case 7 (MEDIA_FIRST_CLICK_ANY) (optional)

Ensure that debug setting `MediaFirstClickInteract` is set to `32767`

Repeat test cases 1-6.

1. Test case 1 should fail
2. Test cases 2-6 should pass

### Case 8 (MEDIA_FIRST_CLICK_BYPASS_MOAP_FLAG) (optional)

Ensure that debug setting `MediaFirstClickInteract` is set to `65535`

Repeat test cases 1-6, there is no pass/fail for this run.

All cubes including B types should exhibit the same first-click interact behavior.
