#!/bin/sh
#
#   put the sign in Demomode.
#   used for testing.
#

#   if start-ctss.save Does not Exist
#      backup start-ctss ... start-ctss.save
#             stop-ctss ... stop-ctss.save
#
#	      cp start-demo start-ctss
#         cp stop-demo stop-ctss
#   else
#       if restore
#            cp start-ctss.save start-ctss
             cp stop-ctss.save stop-ctss
             rm start-ctss.save
             rm stop-ctss.save

