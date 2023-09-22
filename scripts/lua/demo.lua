function popup_and_wait_ok(message)
  args = {{"MESSAGE", message}}
  notif_response = nil
  show_notification("GenericAlertOK", args, "notif_response")
  while not notif_response do
    sleep(0.2)
  end

  local response = notif_response
  return response
end

function demo_environment()
    popup_and_wait_ok("Change Environment")
    run_ui_command("World.EnvSettings", "midnight")
    sleep(2)
    run_ui_command("World.EnvSettings", "sunrise")
    sleep(2)
    run_ui_command("World.EnvSettings", "noon")
    sleep(2)
end

function demo_avatar()
    popup_and_wait_ok("Change Avatar")
    wear_by_name("Greg")
    run_ui_command("Avatar.ResetSelfSkeletonAndAnimations")
    sleep(8)

    wear_by_name("Petrol Sue")
    sleep(8)

end

function demo_ui()
  

  -- adding items to 'Build' menu
  -- popup_and_wait_ok("Extend UI")

  notif_response = nil
  args = {{"MESSAGE", "Extend the UI now?"}}
  show_notification("GenericAlertYesCancel", args, "notif_response")
  while not notif_response do
    sleep(0.2)
  end
  if notif_response ~= 0 then
    popup_and_wait_ok("Exiting")
    return
  end

  menu_name = "BuildTools"
  add_menu_separator(menu_name)

  params = {{"name", "user_sit"}, {"label", "Sit!"},
          {"function", "Self.ToggleSitStand"}}

  add_menu_item(menu_name, params)

  params = {{"name", "user_midnight"}, {"label", "Set night"},
          {"function", "World.EnvSettings"}, {"parameter", "midnight"}}

  add_menu_item(menu_name, params)

  -- adding new custom menu
  new_menu_name = "user_menu"
  params = {{"name", new_menu_name}, {"label", "My Secret Menu"}, {"tear_off", "true"}}
  add_menu(params)

  -- adding new item to the new menu
  params = {{"name", "user_debug"}, {"label", "Console"},
          {"function", "Advanced.ToggleConsole"}, {"parameter", "debug"}}

  add_menu_item(new_menu_name, params)

  -- adding new branch
  new_branch = "user_floaters"
  params = {{"name", new_branch}, {"label", "Open Floater"}, {"tear_off", "true"}}
  add_branch(new_menu_name, params)

  -- adding items to the branch
  params = {{"name", "user_permissions"}, {"label", "Default permissions"},
          {"function", "Floater.ToggleOrBringToFront"}, {"parameter", "perms_default"}}

  add_menu_item(new_branch, params)

  params = {{"name", "user_beacons"}, {"label", "Beacons"},
          {"function", "Floater.ToggleOrBringToFront"}, {"parameter", "beacons"}}

  add_menu_item(new_branch, params)
  sleep(5)

end

function call_once_func()

    demo_environment()
    demo_avatar()
    demo_ui()
end
