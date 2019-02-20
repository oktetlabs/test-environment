BEGIN {
    objective = ""
    has_objective = 0;
    in_test = 0;
    page = ""
    has_page = 0;
    test_name = "TAKE_FROM_C_FILE";
}

function dump() {
    if (in_test && has_objective) {
        if (has_page) {
            page_name = " page=\""page"\"";
        }
        else
            page_name = "";

        gsub(/ [ ]+/, " ", objective);
        gsub(/^ +/, "", objective);
        gsub(" *[\n]+$", "", objective);
        gsub("@a", "", objective);
        gsub("@b", "", objective);
        gsub("@c", "", objective);
        gsub("@e", "", objective);
        gsub("@p", "", objective);

        gsub("<", "\\&lt;", objective);
        gsub(">", "\\&gt;", objective);

        printf("  <test name=\"%s\"%s>\n", test_name, page_name);
        printf("    <objective>%s</objective>\n", objective);
        printf("  </test>\n");

        has_page = 0;
        page = "";
        has_objective = 0;
        objective = "";
        test_name = "TAKE_FROM_C_FILE";
    }
}

/@page/ {
    dump()

    in_test = 1;
}

{
    if (($1 == "*" || $1 == "**") && $2 == "@objective")
    {
        for (i = 3; i <= NF; i++)
            objective = objective " " $i;
        put = 1;
        has_objective = 1;
    }
    else if ($1 == "@objective")
    {
        for (i = 2; i <= NF; i++)
            objective = objective " " $i;
        put = 1;
        has_objective = 1;
    }
    else if (put == 1 && $1 == "*" && NF == 1)
        put = 0;
    else if (put == 1 && NF == 0)
        put = 0;
    else if (put)
    {
        for (i = 2; i <= NF; i++)
            objective = objective " " $i;
    }
}

/@page/ {
    if ($1 == "*" || $1 == "**")
        page = $3
    else
        page = $2

    has_page = 1;
}

/@run_name/ {
    if ($1 == "*" || $1 == "**")
        test_name = $3
    else
        test_name = $2
}

END {
    dump()
}
