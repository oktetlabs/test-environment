#!/bin/bash
# 
# Test Environment
#
# Inverse Test Suite Compiler (light version, for OKTET Labs users only) -
# generates an input file for tsc from correct TS.
#
# Usage: 
#
#    tsc-invert <filename>
#
# Script should be started in the test directory where configure.ac
# is located. 
#
# Copyright (C) 2003-2018 OKTET Labs.
#
# Author Konstantin G. Petrov <Konstantin.Petrov@oktetlabs.ru>
# 
# $Id: $
#

OUTPUTFILE=$1

REFSLIST=''

DIRECTORYTREE=''

CURRENTDIRECTORY=''

WORD=''

GRAPPEDLINE=''

OLDDIRECTORYPATH=''

DIRECTORYPATH=''

SUBDIR=''

function PROCESS_SUBDIR ()
{

    echo '' >> $OUTPUTFILE

    # Grepping the package.dox file header
    if WORD=`grep '/** @page '$CURRENTDIRECTORY' ' ./package.dox`;then

        WORD=${WORD/*'@page '$CURRENTDIRECTORY' '/''}
        echo 'dir '$DIRECTORYPATH'        '$WORD >> $OUTPUTFILE;

    else

        echo 'File ./'$DIRECTORYPATH'/package.dox is absend or corrupted'
        exit 1;

    fi;

    # Grepping the strings matching template ^'-# @ref'
    if REFSLIST=`grep ^'\-\#\ @ref\ ' ./package.dox`;then

        # Cutting out template '-# @ref'
        REFSLIST=${REFSLIST//'-# @ref'/''}

        # REFSLIST processing routine
        for WORD in $REFSLIST;do
            
            # Check if WORD is subdir
            if test -d $WORD;then

                # Goto subdir and recurrent
                cd $WORD
                
                CURRENTDIRECTORY=$WORD
                
                OLDDIRECTORYPATH=$DIRECTORYPATH
                
                DIRECTORYPATH=$DIRECTORYPATH'/'$WORD
                
                # recurrent call of the PROCESSSUBDIR
                PROCESS_SUBDIR
                
                DIRECTORYPATH=$OLDDIRECTORYPATH
                
                # Return from subdir to continue
                cd ../;

            else

                # Search for the *.c files
                # containing the WORD
                if GREPPEDLINE=`grep '/** @page '$WORD' '\
                                    ./*.c`;then

                    GREPPEDLINE=${GREPPEDLINE/*$WORD/''}
                    
                    echo ${WORD/*'-'/''}'        '$GREPPEDLINE\
                         >> $OUTPUTFILE;

                fi;

            fi;
            
        done;

    else

        echo 'file ./'$DIRECTORYPATH'/package.dox is corrupted'
        exit 1;

    fi;

}

# COMMAND line arguments 
if [[ $# > 0 ]];then

    # May be $1 is -h or --help option
    if [[ $1 == '-h' ||\
          $1 == '--help' ||\
          $1 == '-help' ||\
          $1 == 'help' ]];then

        echo './invertest1 [output file name]'
        echo './invertest1 -h|--help|-help|help';

    else 

        OUTPUTFILE=$(pwd)'/'$1;

    fi;

fi

# Grepping list of references in the mainpage.dox
if REFSLIST=`grep ^'\-\ @ref\ ' ./mainpage.dox`; then

    echo '' > $OUTPUTFILE

    # Cutting off the '- @ref' pattern 
    REFSLIST=${REFSLIST//'- @ref'/''}

    # Refslist processing routine
    for DIRECTORYTREE in $REFSLIST;do

        # Entering the DIRECTORYTREE subdir
        if cd $DIRECTORYTREE;then

            # Setting DIRECTORYPATH variable
            DIRECTORYPATH=$DIRECTORYTREE

            # Setting CURRENTDIRECTORY variable
            CURRENTDIRECTORY=$DIRECTORYTREE

            # Recursive subdir processing rouine call
            PROCESS_SUBDIR;

        else

            echo 'The ./'$DIRECTORYTREE' subdir is absend';

        fi

        # Leaving the DIRECTORYTREE subdir
        cd ../;

    done;

else

    # If nothing was grepped in the mainpage.dox
    echo 'mainpage.dox is absend or corrupted';

fi

unset SUBDIR

unset GREPPEDLINE

unset WORD

unset DIRECTORYTREE

unset CURRENTDIRECTORY

unset OLDDIRECTORYPATH

unset DIRECTORYPARTH

unset REFSLIST

unset OUTPUTFILE
