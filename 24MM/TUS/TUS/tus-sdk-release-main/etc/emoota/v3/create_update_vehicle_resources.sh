#!/bin/bash
#TMC CONFIDENTIAL
#$TUSLibId$
#Copyright (C) 2022 TOYOTA MOTOR CORPORATION
#All Rights Reserved.

set -e

if [ $# -ne 1 ]; then
    echo "USAGE: ./create_update_vehicle_resources.sh <path to vehicle information output by init_vehicle.sh>"
    exit 1
fi

# load variables
. $1

# set SDP username and password
if [ -z "$SDP_USERNAME" ]; then
    read -p "SDP_USERNAME:" SDP_USERNAME
    export SDP_USERNAME=$SDP_USERNAME
fi

if [ -z "$SDP_PASSWD" ]; then
    read -sp "SDP_PASSWD:" SDP_PASSWD
    export SDP_PASSWD=$SDP_PASSWD
fi

NEW_PROD_NUMBER1="2.0"
NEW_PROD_NUMBER2="2.5"
NEW_PROD_NUMBER3="3.0"
NEW_PROD_NUMBER4="3.5"
NEW_SYS_ID=$(uuidgen | cut -c-17)
NEW_RXSWINS="RX BBBBB"

###############
# Create subsystem configurations
###############
echo "update sybsystem configuration new_sys_id: $NEW_SYS_ID, new_rxswins:$NEW_RXSWINS"
CREATE_NEW_SUBSYSCONF_RES=$(
    tus pdp sdpv3 rest update-subsysconf --print-json |
    jq --argjson HWCOMP_ID "$HWCOMP_ID1" '.components[0].id|=$HWCOMP_ID' |
    jq --argjson HWCOMP_ID "$HWCOMP_ID2" '.components[1].id|=$HWCOMP_ID' |
    jq --argjson HWCOMP_ID "$HWCOMP_ID3" '.components[2].id|=$HWCOMP_ID' |
    jq --argjson HWCOMP_ID "$HWCOMP_ID4" '.components[3].id|=$HWCOMP_ID' |
    jq --arg PROD_NUMBER "$NEW_PROD_NUMBER1" '.components[0].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID1" '.components[0].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq --arg PROD_NUMBER "$NEW_PROD_NUMBER2" '.components[1].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID2" '.components[1].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq --arg PROD_NUMBER "$NEW_PROD_NUMBER3" '.components[2].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID3" '.components[2].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq --arg PROD_NUMBER "$NEW_PROD_NUMBER4" '.components[3].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID4" '.components[3].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq '.components[0].softwareInfos[0].reproDeliveryMethod|="streaming"' |
    jq '.components[1].softwareInfos[0].reproDeliveryMethod|="streaming"' |
    jq '.components[2].softwareInfos[0].reproDeliveryMethod|="streaming"' |
    jq '.components[3].softwareInfos[0].reproDeliveryMethod|="streaming"' |
    jq '.components[1].softwareInfos[0].note|=""' |
    jq '.components[2].softwareInfos[0].note|=""' |
    jq '.components[3].softwareInfos[0].note|=""' |
    tus pdp sdpv3 rest create-subsysconf $SUBSYS_ID $NEW_SYS_ID --compare-sysid $SYS_ID --rxswins $NEW_RXSWINS --json -
)
NEW_SUBSYSCONF_ID=$(echo $CREATE_NEW_SUBSYSCONF_RES | jq '.id')
NEW_ELEMENT_SWCONFIG_ID1=$(echo $CREATE_NEW_SUBSYSCONF_RES | jq '.components[0].softwareInfos[0].elementConfigSwId')
NEW_ELEMENT_SWCONFIG_ID2=$(echo $CREATE_NEW_SUBSYSCONF_RES | jq '.components[1].softwareInfos[0].elementConfigSwId')
NEW_ELEMENT_SWCONFIG_ID3=$(echo $CREATE_NEW_SUBSYSCONF_RES | jq '.components[2].softwareInfos[0].elementConfigSwId')
NEW_ELEMENT_SWCONFIG_ID4=$(echo $CREATE_NEW_SUBSYSCONF_RES | jq '.components[3].softwareInfos[0].elementConfigSwId')

###############
# create new vehicle design
###############
echo "create vehicle design for update. new_subsystemconfiguration_id: $NEW_SUBSYSCONF_ID, prev_vehicle_design_id:$VDESIGN_ID"
CREATE_NEW_VDESIGN_ID_RES=$(
    tus pdp sdpv3 rest create-vdesign --print-json |
    jq --argjson SUBSYSCONF_ID $NEW_SUBSYSCONF_ID '.subsystems[0].subsystemConfigurationId|=$SUBSYSCONF_ID' |
    tus pdp sdpv3 rest create-vdesign $CREATE_VTYPE_ID --prev-vdesign-id $VDESIGN_ID --json -
)
NEW_VDESIGN_ID=$(echo $CREATE_NEW_VDESIGN_ID_RES | jq '.id')
tus pdp sdpv3 rest approve-vdesign $NEW_VDESIGN_ID

###############
# store temporal values
###############
echo "create update_vehicle_information for next script..."
echo VIN=$VIN >update_vehicle_information
echo MAKECODE=$MAKECODE >>update_vehicle_information
echo CREATE_VTYPE_ID=$CREATE_VTYPE_ID >>update_vehicle_information
echo VDESIGN_ID=$VDESIGN_ID >>update_vehicle_information
echo NEW_VDESIGN_ID=$NEW_VDESIGN_ID >>update_vehicle_information
echo ELEMENT_SWCONFIG_ID1=$ELEMENT_SWCONFIG_ID1 >>update_vehicle_information
echo NEW_ELEMENT_SWCONFIG_ID1=$NEW_ELEMENT_SWCONFIG_ID1 >>update_vehicle_information
echo NEW_ELEMENT_SWCONFIG_ID2=$NEW_ELEMENT_SWCONFIG_ID2 >>update_vehicle_information
echo NEW_ELEMENT_SWCONFIG_ID3=$NEW_ELEMENT_SWCONFIG_ID3 >>update_vehicle_information
echo NEW_ELEMENT_SWCONFIG_ID4=$NEW_ELEMENT_SWCONFIG_ID4 >>update_vehicle_information
echo SYS_ID=$SYS_ID >>update_vehicle_information
echo NEW_SYS_ID=$NEW_SYS_ID >>update_vehicle_information

echo "create TUP by using new vehicle design id:$NEW_VDESIGN_ID"
