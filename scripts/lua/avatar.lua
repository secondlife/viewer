function call_once_func()
  run_ui_command("World.EnvSettings", "midnight")
  sleep(1)
  run_ui_command("World.EnvSettings", "noon")
  sleep(1)
  wear_by_name("* AVL")
  run_ui_command("Avatar.ResetSelfSkeletonAndAnimations")
  sleep(5)
  wear_by_name("* Elephant")
  sleep(5)
  play_animation("Elephant_Fly");
  sleep(5)
  play_animation("Elephant_Fly",1);
end