#############################################################################
#   Copyright (C) 2004-2019 by Thomas Fischer <fischer@unix-ag.uni-kl.de>   #
#                                                                           #
#   This program is free software; you can redistribute it and/or modify    #
#   it under the terms of the GNU General Public License as published by    #
#   the Free Software Foundation; either version 2 of the License, or       #
#   (at your option) any later version.                                     #
#                                                                           #
#   This program is distributed in the hope that it will be useful,         #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of          #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           #
#   GNU General Public License for more details.                            #
#                                                                           #
#   You should have received a copy of the GNU General Public License       #
#   along with this program; if not, see <https://www.gnu.org/licenses/>.   #
#############################################################################

# Inspired by:
# https://stackoverflow.com/questions/3780667/use-cmake-to-get-build-time-svn-revision

if(DEFINED ENV{GIT_REV} AND DEFINED ENV{GIT_BRANCH} AND NOT("${GIT_REV}" STREQUAL "" OR "${GIT_BRANCH}" STREQUAL ""))
    message (STATUS "Git information set by environment variables GIT_REV and GIT_BRANCH")
    set (GIT_REV $ENV{GIT_REV})
    set (GIT_BRANCH $ENV{GIT_BRANCH})
    set (GIT_COMMIT_COUNT "0")
else()
    set(GIT_REV "")
    set(GIT_BRANCH "")
    set(GIT_COMMIT_COUNT "0")

    if(EXISTS ${SOURCE_DIR}/.git)
        # Git
        find_program( GIT_EXECUTABLE NAMES git.bat git ) # for Windows, "git.bat" must be found before "git"
        if(GIT_EXECUTABLE)
            execute_process (
                WORKING_DIRECTORY
                "${SOURCE_DIR}"
                COMMAND
                ${GIT_EXECUTABLE} rev-parse --short HEAD
                OUTPUT_VARIABLE GIT_REV
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            execute_process (
                WORKING_DIRECTORY
                "${SOURCE_DIR}"
                COMMAND
                ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
                OUTPUT_VARIABLE GIT_BRANCH
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            execute_process (
                WORKING_DIRECTORY
                "${SOURCE_DIR}"
                COMMAND
                ${GIT_EXECUTABLE} rev-list --count HEAD
                OUTPUT_VARIABLE GIT_COMMIT_COUNT
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
        else()
            message( "No Git executable" )
        endif()
    else()
        message( "Not a Git source directory" )
    endif()
endif()

# write a header file defining VERSION
file(
    WRITE
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
    "/// This file has been automatically generated by 'getgit.cmake'.\n/// Do not edit or modify it.\n\n"
)
file(
    APPEND
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
    "#ifndef KBIBTEX_GIT_INFO_H\n"
)
file(
    APPEND
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
    "#define KBIBTEX_GIT_INFO_H\n"
)

if("${GIT_REV}" STREQUAL "" OR "${GIT_BRANCH}" STREQUAL "")
    set(GIT_REV "")
    set(GIT_BRANCH "")
    set(GIT_COMMIT_COUNT "0")
    message(
        STATUS
        "Source does not come from a Git checkout or determining the Git revision or branch failed"
    )
    file(
        APPEND
        "${BINARY_DIR}/kbibtex-git-info.h.tmp"
        "/// This source code does not come from a Git checkout or\n/// determining the Git revision or branch failed.\n/// Please consider setting environment variables GIT_REV and\n/// GIT_BRANCH before running the build tool (make, ninja, ...).\n\n"
    )
else()
    message(
        STATUS
        "Git revision is "
        ${GIT_REV}
        "\nGit branch is "
        ${GIT_BRANCH}
        "\nGit commit count is "
        ${GIT_COMMIT_COUNT}
    )
endif()

file(
    APPEND
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
    "#define KBIBTEX_GIT_REV_STRING \"${GIT_REV}\"\n"
)
file(
    APPEND
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
    "#define KBIBTEX_GIT_BRANCH_STRING \"${GIT_BRANCH}\"\n"
)
file(
    APPEND
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
    "#define KBIBTEX_GIT_COMMIT_COUNT ${GIT_COMMIT_COUNT}\n"
)
if("${GIT_REV}" STREQUAL "" OR "${GIT_BRANCH}" STREQUAL "")
    file(
        APPEND
        "${BINARY_DIR}/kbibtex-git-info.h.tmp"
        "#define KBIBTEX_GIT_INFO_STRING \"\"\n"
    )
else()
    file(
        APPEND
        "${BINARY_DIR}/kbibtex-git-info.h.tmp"
        "#define KBIBTEX_GIT_INFO_STRING \"${GIT_REV} (${GIT_BRANCH}, ${GIT_COMMIT_COUNT} commits in history)\"\n"
    )
endif()
file(
    APPEND
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
    "#endif // KBIBTEX_GIT_INFO_H\n"
)

if(
    EXISTS
    "${BINARY_DIR}/kbibtex-git-info.h.tmp"
)
    execute_process(
        COMMAND
        ${CMAKE_COMMAND}
        -E
        copy_if_different
        "kbibtex-git-info.h.tmp"
        "kbibtex-git-info.h"
        WORKING_DIRECTORY
        "${BINARY_DIR}"
    )
    execute_process(
        COMMAND
        ${CMAKE_COMMAND}
        -E
        remove
        "kbibtex-git-info.h.tmp"
        WORKING_DIRECTORY
        "${BINARY_DIR}"
    )
else()
    message(
        STATUS
        "${BINARY_DIR}/kbibtex-git-info.h.tmp does not exist"
    )
endif()
