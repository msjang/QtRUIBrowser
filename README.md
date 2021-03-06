# QT RUI Browser README

These instructions assume you've build Qt5 and WebKit, following our standard
instructions.

## Get Source

Download the repo using either SSH (if you're a CableLabs employee) or HTTP:

    # recommended location
    cd ~/workspace

    git clone git@github.com:cablelabs/QtRUIBrowser.git
    # or
    git clone https://github.com/cablelabs/QtRUIBrowser.git

    cd QtRUIBrowser

## Build

    # Your path should be setup already from building WebKit, like so:
    export QTDIR=~/workspace/qt5/qtbase
    export PATH=$QTDIR/bin:$PATH

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

  * Clicking on the UI entry.
  * Entering the number via the keyboard.
  * Scrolling up/down until the desired UI is highlighted, then pressing enter.

Return to the Navigation Page via ctrl/escape key or back button.

NOTE: This was changed from just escape because escape is used to exit full screen mode.
