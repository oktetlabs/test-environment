
proc check_rgt_entity_filter { entity user result } {
    if {[string compare [rgt_entity_filter $entity $user] $result] != 0} {
        puts "$entity $user $result"
        exit 1;
    }
}

