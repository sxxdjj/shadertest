#!/usr/bin/python
# build_native.py
# Build native codes
# 
# Please use cocos console instead


import sys
import os, os.path
import shutil
from optparse import OptionParser

def build(opts):
    
    if opts.ndk_clean == 'clean':
        app_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "app")
        command_clean = 'ndk-build %s -C %s' % (opts.ndk_clean, app_dir)
        if os.system(command_clean) != 0:
            raise Exception("NDK_ROOT is not in PATH!")
    if opts.build_mode is None:
    	  opts.build_mode = 'debug'
    elif opts.build_mode != 'release':
        opts.build_mode = 'debug'

    if opts.android_platform is None:
        opts.android_platform = 'android-23'

    command = 'cocos compile -p android --android-studio -op %s -m %s' % (opts.android_platform, opts.build_mode)
    if os.system(command) != 0:
        raise Exception("Android platform for project [ " + opts.android_platform + " ] fails!")

# -------------- main --------------
if __name__ == '__main__':

    parser = OptionParser()
    parser.add_option("-c", "--clean", dest="ndk_clean")
    parser.add_option("-p", "--platform", dest="android_platform")
    parser.add_option("-m", "--mode", dest="build_mode")
    (opts, args) = parser.parse_args()
    
    print "Please use cocos console instead.\n"
    
    build(opts)
