#!/usr/bin/perl -w


# strg_storage_to_local(storage_config, st_obj_name, local_dirname, recursive, 
# update)
#
# copies the files, directory (possibly with subdirectories) from storage
# into specified directory in the local file system.
#
# PARAMETERS:
#     storage_config - The string with information about the storage to be
#                      used.
#     st_obj_name    - String with filename or directory to be copied.
#     local_dirname  - String with path to local destination directory.
#     recursive      - Recursive flag: 0/undef     - non-recursive mode,
#                                      other value - recursive mode.
#     update         - Update flag::   0/undef     - plain copying
#                                      other value - copying only new/fresh
#                                                    files/directories
#
sub strg_storage_to_local
{
}


# strg_local_to_storage(storage_config, local_dirname, st_obj_name, recursive
# update)
#
# copies the files, directory (possibly with subdirectories) from local
# file system into the storage
#
# PARAMETERS:
#     storage_config - The string with information about the storage to be 
#                      used.
#     local_dirname  - String with filename or directory to be copied in the
#                      local filesystem.
#     st_obj_name    - Destination directory in the storage.
#     recursive      - Recursive flag: 0/undef     - non-recursive mode,
#                                      other value - recursive mode.
#     update         - Update flag::   0/undef     - plain copying
#                                      other value - copying only new/fresh
#                                                    files/directories
#
sub strg_local_to_storage
{
}

