
set all_log_levels [list  ERROR WARN RING INFO VERB ENTRY_EXIT]

proc level_str2int { level_str } {
    switch $level_str {
      "ERROR" { return 1 }
      "WARN" { return 2 }
      "RING" { return 4 }
      "INFO" { return 8 }
      "VERB" { return 16 }
      "ENTRY_EXIT" { return 32 }
      
      default {
          error "Unknown level $level_str";
      }
    }
}

proc check_rgt_entity_filter { entity user level result } {
    set rc [rgt_entity_filter $entity $user [level_str2int $level]]

    if {[string compare $rc $result] != 0} {
        puts "$entity $user $level $result"
        exit 1;
    }
}

proc check_rgt_branch_filter { branch result } {
    set rc [rgt_branch_filter $branch]

    if {[string compare $rc $result] != 0} {
        puts "$branch $result"
        exit 1;
    }
}

proc check_rgt_duration_filter { node duration result } {
    set rc [rgt_duration_filter $node $duration]

    if {[string compare $rc $result] != 0} {
        puts "$node $duration $result"
        exit 1;
    }
}

if {[info exists any_log_level] && $any_log_level == 1} {
  if {![info exists test_cases_fail] && ![info exists test_cases_pass]} {
    error "You should provide test_cases_fail or test_cases_pass list"
  }

  foreach { level } $all_log_levels {
    if {[info exists test_cases_fail]} {
      foreach { entity user } $test_cases_fail {
        check_rgt_entity_filter $entity $user $level "fail"
      }
    }

    if {[info exists test_cases_pass]} {
      foreach { entity user } $test_cases_pass {
        check_rgt_entity_filter $entity $user $level "pass"
      }
    }
  }
}
