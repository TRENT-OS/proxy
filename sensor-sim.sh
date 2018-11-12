#!/bin/bash -eu

# "-n" disbaled output of the trailing newline
# "-e" enable interpretation of backslash escape sequences
#
# send MQTT message PUBLISH with QS=1 and message ID 0x0002
echo -ne "\x32\x29\x00\x21devices/test_dev/messages/events/\x00\x02ciao" | nc localhost 7999
