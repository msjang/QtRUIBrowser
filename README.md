# QT RUI Browser README

These instructions assume you've build Qt5 and WebKit, following our standard
instructions.

## Build

    # With the latest WebKit distro, the following variable needs to be set before running qmake:
    # Note: This probably shouldn't be necessary if your PATH is correct?
    export QMAKEPATH=<yourPath>/WebKit/Tools/qmake

    # This should point to your WebKit checkout
    export WEBKIT_ROOT=~/workspace/webkit

    # This makes a debug version
    qmake QtRUIBrowser.pro
    
    # This makes a release version
    qmake QtRUIBrowser.pro CONFIG+=release CONFIG-=debug
    make

## Run

    # Make sure this points to wherever you built WebKit
    # The binary will be in either bin/debug or bin/release based on how you built
    LD_LIBRARY_PATH=~/workspace/webkit/WebKitBuild/Debug/lib bin/debug/QtRUIBrowser

## Operation / Testing

    Test using the CableLabs RUIServer (java mock RUI Servers, see the related README)

    Select a RUI by:

      - clicking on the UI entry
      - entering the number via keyboard
      - scrolling up/down until desired UI highlighted, then press enter

    Return to the Navigation Page via ctrl/escape key or back button.
    NOTE: This was changed from just escape because escape is used to exit full screen mode

