#!/bin/bash
set -ex

# Clean up previous runs
/usr/bin/killall -q MainWindows || true
/usr/bin/killall -q Xvfb || true
rm -f /tmp/.X0-lock
rm -f layout.xml

# Start Xvfb
export DISPLAY=:0
/usr/bin/Xvfb :0 -screen 0 1024x768x16 &
XVFB_PID=$!
# Kill Xvfb on exit
trap "kill $XVFB_PID" EXIT

# Wait for Xvfb to start
sleep 2

# Start the application
/app/build/MainWindows &
APP_PID=$!

# Wait for the application window to appear
echo "Waiting for application to initialize..."
sleep 5

# Send a quit signal to the app, which should trigger saveLayout()
echo "Closing application to trigger layout save..."
kill -TERM $APP_PID
sleep 2 # wait for it to save and exit

# Check if layout.xml was created and contains splitter info
if [ -f layout.xml ]; then
    echo "layout.xml found."
    if grep -q "SplitterStates" layout.xml; then
        echo "OK: Splitter states found in layout.xml."
        if grep -q "splitter0" layout.xml; then
             echo "OK: Splitter 'splitter0' found in layout.xml."
        else
             echo "FAIL: Splitter 'splitter0' NOT found in layout.xml."
             cat layout.xml
             exit 1
        fi
    else
        echo "FAIL: Splitter states not found in layout.xml."
        cat layout.xml
        exit 1
    fi
else
    echo "FAIL: layout.xml not found."
    exit 1
fi

echo "Test successful."
rm layout.xml
rm test.sh