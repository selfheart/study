#!/bin/bash
#TMC CONFIDENTIAL
#$TUSLibId$
#Copyright (C) 2022 TOYOTA MOTOR CORPORATION
#All Rights Reserved.

set -e

if [ $# -ne 2 ]; then
    echo "USAGE: ./create_campaigns.sh <path to update_vehicle_information> <TUP filepath>"
    exit 1
fi

# load variables
. $1

TUP_FILE_PATH=$2
TUP_FILE_NAME=$(basename $TUP_FILE_PATH)
DL_METADATA_PATH="${TUP_FILE_NAME}_dl_meta.txt"
echo $TUP_FILE_NAME >$DL_METADATA_PATH
REPRO_METADATA_PATH="${TUP_FILE_NAME}_repro_meta.txt"
echo "dummy repro policy metadata" >$REPRO_METADATA_PATH

# set SDP username and password
if [ -z "$SDP_USERNAME" ]; then
    read -p "SDP_USERNAME:" SDP_USERNAME
    export SDP_USERNAME=$SDP_USERNAME
fi

if [ -z "$SDP_PASSWD" ]; then
    read -sp "SDP_PASSWD:" SDP_PASSWD
    export SDP_PASSWD=$SDP_PASSWD
fi

####################
# create change event
####################
CHANGE_EVENT_ID=$MAKECODE

echo "create change event, change_event_id: $CHANGE_EVENT_ID"
CHANGE_EVENT_MODIFICATIONS=$(
    tus pdp sdpv3 rest create-chevent --print-json |
    jq --arg START_SYS_ID $SYS_ID --arg RESULT_SYS_ID $NEW_SYS_ID --argjson ELEMENT_SWCONFIG_ID1 $ELEMENT_SWCONFIG_ID1 \
        '.modifications[0].starts[0].startSysId|=$START_SYS_ID |
        .modifications[0].starts[0].subTargetReproTypes[0].elementConfigSwId|=$ELEMENT_SWCONFIG_ID1 |
        .modifications[0].resultSysId|=$RESULT_SYS_ID'
)
CHANGE_EVENT_RES=$(echo $CHANGE_EVENT_MODIFICATIONS | tus pdp sdpv3 rest create-chevent $CHANGE_EVENT_ID --reason-content "content" --json -)
CHANGE_EVENT_SERVER_ID=$(echo $CHANGE_EVENT_RES | jq '.id')

####################
# create diff.json
####################
echo "create diff.json"
tus pdp sdpv3 rest create-subsysdiff --print-json |
jq --arg START_SYS_ID $SYS_ID --arg RESULT_SYS_ID $NEW_SYS_ID --argjson ELEMENT_SWCONFIG_ID1 $ELEMENT_SWCONFIG_ID1 \
    '.systems[0].starts[0].startSysId|=$START_SYS_ID |
        .systems[0].starts[0].subTargetReproTypes[0].elementConfigSwId|=$ELEMENT_SWCONFIG_ID1 |
        .systems[0].resultSysId|=$RESULT_SYS_ID' |
tus pdp sdpv3 rest create-subsysdiff --json - --out diff.json

CHANGE_EVENT_PKG_RES=$(tus pdp sdpv3 rest create-cheventpkg $MAKECODE ./diff.json)
CHANGE_EVENT_PKG_ID=$(echo $CHANGE_EVENT_PKG_RES | jq '.id')

####################
# upload package and update change event to link
###################
echo "upload tup: $TUP_FILE_PATH, change event package id:$CHANGE_EVENT_PKG_ID"
tus pdp sdpv3 ul-pkg $CHANGE_EVENT_PKG_ID $TUP_FILE_PATH

echo "upload dl_metadata: $DL_METADATA_PATH, change event package id:$CHANGE_EVENT_PKG_ID"
tus pdp sdpv3 ul-pkg $CHANGE_EVENT_PKG_ID $DL_METADATA_PATH --file-type DOWNLOAD_METADATA
# tus pdp sdpv3 ul-pkg $CHANGE_EVENT_PKG_ID $REPRO_METADATA_PATH --file-type REPRO_METADATA --approve
echo "upload repro_metadata: $REPRO_METADATA_PATH, change event package id:$CHANGE_EVENT_PKG_ID"
UPLOAD_LOCATION=$(tus pdp sdpv3 ul-pkg $CHANGE_EVENT_PKG_ID $REPRO_METADATA_PATH --file-type REPRO_METADATA --async_ops | jq '.upload_location')
# check status
while true; do
    UPLOAD_STATUS=$(tus pdp sdpv3 get-pkg-status $UPLOAD_LOCATION | jq '.status')
    if [ "$UPLOAD_STATUS" == "\"FAILED\"" ]; then
        echo "Processing uploaded pkg failed on pdp. exitting script."
        exit 1
    elif [ "$UPLOAD_STATUS" == "\"CANCELLED\"" ]; then
        echo "Processing uploaded pkg cancelled on pdp. exitting script."
        exit 1
    elif [ "$UPLOAD_STATUS" == "\"COMPLETED\"" ]; then
        echo "Processing uploaded pkg completed on pdp."
        break
    elif [ "$UPLOAD_STATUS" == "\"PROCESSING\"" ]; then
        echo "Processing uploaded pkg in progress."
        sleep 3
    else
        echo "Unknown error occured. exitting script."
        exit 1
    fi
done
tus pdp sdpv3 approve-pkg $CHANGE_EVENT_PKG_ID

# create-chevent.json is needed even if it is passed when creating change event
echo $CHANGE_EVENT_MODIFICATIONS | tus pdp sdpv3 rest update-chevent $CHANGE_EVENT_SERVER_ID $CHANGE_EVENT_ID --pkg-id $CHANGE_EVENT_PKG_ID --json -
tus pdp sdpv3 rest submit-chevent $CHANGE_EVENT_SERVER_ID
tus pdp sdpv3 rest approve-chevent $CHANGE_EVENT_SERVER_ID

####################
# create campaign
####################
CAMPAIGN_NAME=$MAKECODE
CAMPAIGN_RES=$(tus pdp sdpv3 rest create-campaign $CAMPAIGN_NAME)
CAMPAIGN_ID=$(echo $CAMPAIGN_RES | jq '.id')
echo "create campaign, name:$CAMPAIGN_NAME"
echo "update campaign, id:$CAMPAIGN_ID"
tus pdp sdpv3 rest update-campaign $CAMPAIGN_ID --chevents $CHANGE_EVENT_SERVER_ID 0
tus pdp sdpv3 rest update-campaign $CAMPAIGN_ID --vins $VIN
tus pdp sdpv3 rest update-campaign $CAMPAIGN_ID --vtypes $CREATE_VTYPE_ID
# you can add localized message to the campaign by the command below
# tus pdp sdpv3 rest update-campaign $CAMPAIGN_ID --message-file ./sample_localized_message.csv
tus pdp sdpv3 rest submit-campaign $CAMPAIGN_ID
tus pdp sdpv3 rest approve-campaign $CAMPAIGN_ID
