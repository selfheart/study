#!/bin/bash
#TMC CONFIDENTIAL
#$TUSLibId$
#Copyright (C) 2022 TOYOTA MOTOR CORPORATION
#All Rights Reserved.

set -e

#################
# Some parameters defined here.
#################

# public key path (DER format)
PUB_KEY_DER="./master_pubkey.der"

# Some parameters are used same as MAKECODE.
# If you want to use unique parameters, please read this script and update them.
MAKECODE=$(uuidgen | cut -c-18)
# VIN shall be just 17 byte.
VIN=$(uuidgen | cut -c-17)

# set SDP username and password
if [ -z "$SDP_USERNAME" ]; then
    read -p "SDP_USERNAME:" SDP_USERNAME
    export SDP_USERNAME=$SDP_USERNAME
fi

if [ -z "$SDP_PASSWD" ]; then
    read -sp "SDP_PASSWD:" SDP_PASSWD
    export SDP_PASSWD=$SDP_PASSWD
fi

#################
# create vehicle
#################
REGION="JP"
MODEL=$MAKECODE
echo "create vehicle type make_code: $MAKECODE"
CREATE_VTYPE_RES=$(tus pdp sdpv3 rest create-vtype $MAKECODE --model-code $MODEL --region $REGION)
CREATE_VTYPE_ID=$(echo $CREATE_VTYPE_RES | jq '.[0].id')

#################
# create subsystem
#################
SYS_ID=$MAKECODE
SYS_NAME=$MAKECODE
RXSWINS="RX AAAAA"
echo "create subsystem sys_id: $SYS_ID, sys_name: $SYS_NAME"
CREATE_SUBSYS_RES=$(tus pdp sdpv3 rest create-subsys $SYS_NAME $SYS_ID --rxswins $RXSWINS)
SUBSYS_ID=$(echo $CREATE_SUBSYS_RES | jq '.id')
SUBSYSCONF_ID=$(echo $CREATE_SUBSYS_RES | jq '.subsystemConfiguration.id')

#################
# create hardware component
#################
TARGET_ID1="01000"
HWCOMP_NAME1=$MAKECODE
PART_NUM1=$(uuidgen | cut -c-16)
echo "create hardware component target_id: $TARGET_ID1, hardware_component_name: $HWCOMP_NAME1, part_number: $PART_NUM1"
CREATE_HWCOMP_RES1=$(tus pdp sdpv3 rest create-hwcomp $TARGET_ID1 $HWCOMP_NAME1 --part-num $PART_NUM1)

#################
# create second hardware component
#################
TARGET_ID2="01001"
HWCOMP_NAME2=$(uuidgen | cut -c-18)
PART_NUM2=$(uuidgen | cut -c-16)
echo "create hardware component target_id: $TARGET_ID2, hardware_component_name: $HWCOMP_NAME2, part_number: $PART_NUM2"
CREATE_HWCOMP_RES2=$(tus pdp sdpv3 rest create-hwcomp $TARGET_ID2 $HWCOMP_NAME2 --part-num $PART_NUM2)

#################
# create third hardware component
#################
TARGET_ID3="01010"
HWCOMP_NAME3=$(uuidgen | cut -c-18)
PART_NUM3=$(uuidgen | cut -c-16)
echo "create hardware component target_id: $TARGET_ID3, hardware_component_name: $HWCOMP_NAME3, part_number: $PART_NUM3"
CREATE_HWCOMP_RES3=$(tus pdp sdpv3 rest create-hwcomp $TARGET_ID3 $HWCOMP_NAME3 --part-num $PART_NUM3)

#################
# create fourth hardware component
#################
TARGET_ID4="01011"
HWCOMP_NAME4=$(uuidgen | cut -c-18)
PART_NUM4=$(uuidgen | cut -c-16)
echo "create hardware component target_id: $TARGET_ID4, hardware_component_name: $HWCOMP_NAME4, part_number: $PART_NUM4"
CREATE_HWCOMP_RES4=$(tus pdp sdpv3 rest create-hwcomp $TARGET_ID4 $HWCOMP_NAME4 --part-num $PART_NUM4)

#################
# link subsystem and hardware component
#################
HWCOMP_ID1=$(echo $CREATE_HWCOMP_RES1 | jq ".id")
HWCOMP_ID2=$(echo $CREATE_HWCOMP_RES2 | jq ".id")
HWCOMP_ID3=$(echo $CREATE_HWCOMP_RES3 | jq ".id")
HWCOMP_ID4=$(echo $CREATE_HWCOMP_RES4 | jq ".id")
PROD_NUMBER1="1.0"
PROD_NUMBER2="1.5"
PROD_NUMBER3="2.0"
PROD_NUMBER4="3.0"
SUBTARGET_ID1=$(uuidgen | cut -c-5)
SUBTARGET_ID2=$(uuidgen | cut -c-5)
SUBTARGET_ID3=$(uuidgen | cut -c-5)
SUBTARGET_ID4=$(uuidgen | cut -c-5)
UPDATE_SUBSYSCONF_RES=$(
    tus pdp sdpv3 rest update-subsysconf --print-json |
    jq --argjson HWCOMP_ID1 "$HWCOMP_ID1" '.components[0].id|=$HWCOMP_ID1' |
    jq --argjson HWCOMP_ID2 "$HWCOMP_ID2" '.components[1].id|=$HWCOMP_ID2' |
    jq --argjson HWCOMP_ID3 "$HWCOMP_ID3" '.components[2].id|=$HWCOMP_ID3' |
    jq --argjson HWCOMP_ID4 "$HWCOMP_ID4" '.components[3].id|=$HWCOMP_ID4' |
    jq --arg PROD_NUMBER "$PROD_NUMBER1" '.components[0].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID1" '.components[0].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq --arg PROD_NUMBER "$PROD_NUMBER2" '.components[1].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID2" '.components[1].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq --arg PROD_NUMBER "$PROD_NUMBER3" '.components[2].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID3" '.components[2].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq --arg PROD_NUMBER "$PROD_NUMBER4" '.components[3].softwareInfos[0].productNumber|=$PROD_NUMBER' |
    jq --arg SUBTARGET_ID "$SUBTARGET_ID4" '.components[3].softwareInfos[0].subTargetId|=$SUBTARGET_ID' |
    jq '.components[1].softwareInfos[0].reproDeliveryMethod|="streaming"' |
    jq '.components[2].softwareInfos[0].reproDeliveryMethod|="streaming"' |
    jq '.components[3].softwareInfos[0].reproDeliveryMethod|="streaming"' |
    jq '.components[1].softwareInfos[0].note|=""' |
    jq '.components[2].softwareInfos[0].note|=""' |
    jq '.components[3].softwareInfos[0].note|=""' |
    tus pdp sdpv3 rest update-subsysconf $SUBSYSCONF_ID --rxswins $RXSWINS --json -
)
ELEMENT_SWCONFIG_ID1=$(echo $UPDATE_SUBSYSCONF_RES | jq '.components[0].softwareInfos[0].elementConfigSwId')
ELEMENT_SWCONFIG_ID2=$(echo $UPDATE_SUBSYSCONF_RES | jq '.components[1].softwareInfos[0].elementConfigSwId')
ELEMENT_SWCONFIG_ID3=$(echo $UPDATE_SUBSYSCONF_RES | jq '.components[2].softwareInfos[0].elementConfigSwId')
ELEMENT_SWCONFIG_ID4=$(echo $UPDATE_SUBSYSCONF_RES | jq '.components[3].softwareInfos[0].elementConfigSwId')

#################
# create vehicle design
#################
CREATE_VDESIGN_RES=$(
    tus pdp sdpv3 rest create-vdesign --print-json |
    jq --argjson SUBSYSCONF_ID $SUBSYSCONF_ID '.subsystems[0].subsystemConfigurationId|=$SUBSYSCONF_ID' |
    tus pdp sdpv3 rest create-vdesign $CREATE_VTYPE_ID --status "PENDING" --json -
)
VDESIGN_ID=$(echo $CREATE_VDESIGN_RES | jq '.id')
tus pdp sdpv3 rest approve-vdesign $VDESIGN_ID

#################
# create vehicle (publish VIN)
#################
# prepare key infos
KEY_TYPE="RSA"
# write to pubkey from DER
PUB_KEY="./pubkey"
PUBLIC_KEY_ID=$(sha256sum ${PUB_KEY_DER} | awk '{print $1}')
xxd -c 10000 -p $PUB_KEY_DER | head -c -1 >$PUB_KEY

EVENT_ID=$MAKECODE
ELEMENT_ID1=$MAKECODE
ELEMENT_ID2=$(uuidgen | cut -c-18)
ELEMENT_ID3=$(uuidgen | cut -c-18)
ELEMENT_ID4=$(uuidgen | cut -c-18)
EQUIPPED_SYS=$SYS_NAME
echo "create vehicle VIN:$VIN"
tus pdp sdpv3 rest create-vehicle --print-json |
    jq --arg VIN "$VIN" --arg MAKECODE "$MAKECODE" --arg MODEL "$MODEL" --arg TRIM "$MAKECODE" --arg REGION "$REGION" \
        --arg EVENT_ID "$EVENT_ID" --arg RXSWINS "$RXSWINS" \
        --arg PROD_NUMBER1 "$PROD_NUMBER1" --arg SUBTARGET_ID1 "$SUBTARGET_ID1" \
        --arg PROD_NUMBER2 "$PROD_NUMBER2" --arg SUBTARGET_ID2 "$SUBTARGET_ID2" \
        --arg PROD_NUMBER3 "$PROD_NUMBER3" --arg SUBTARGET_ID3 "$SUBTARGET_ID3" \
        --arg PROD_NUMBER4 "$PROD_NUMBER4" --arg SUBTARGET_ID4 "$SUBTARGET_ID4" \
        --arg PUBLIC_KEY_ID "$PUBLIC_KEY_ID" --arg KEY_TYPE "$KEY_TYPE" \
        --arg TARGET_ID1 "$TARGET_ID1" --arg PART_NUM1 "$PART_NUM1" --arg ELEMENT_ID1 "$ELEMENT_ID1" \
        --arg TARGET_ID2 "$TARGET_ID2" --arg PART_NUM2 "$PART_NUM2" --arg ELEMENT_ID2 "$ELEMENT_ID2" \
        --arg TARGET_ID3 "$TARGET_ID3" --arg PART_NUM3 "$PART_NUM3" --arg ELEMENT_ID3 "$ELEMENT_ID3" \
        --arg TARGET_ID4 "$TARGET_ID4" --arg PART_NUM4 "$PART_NUM4" --arg ELEMENT_ID4 "$ELEMENT_ID4" \
        --arg EQUIPPED_SYS "$EQUIPPED_SYS" \
        '.vehicleInfos[0].vin|=$VIN |
        .vehicleInfos[0].vehicleMake|=$MAKECODE |
        .vehicleInfos[0].vehicleModel|=$MODEL |
        .vehicleInfos[0].vehicleTrim|=$TRIM |
        .vehicleInfos[0].region|=$REGION |
        .vehicleInfos[0].eventId|=$EVENT_ID |
        .vehicleInfos[0].rxswins[0]|=$RXSWINS |
        .vehicleInfos[0].primaryElementId|=$ELEMENT_ID1 |
        .vehicleInfos[0].equippedSystems[0]|=$EQUIPPED_SYS |
        .vehicleInfos[0].vehicleConfigurations[0].elementId|=$ELEMENT_ID1 |
        .vehicleInfos[0].vehicleConfigurations[0].targetId|=$TARGET_ID1 |
        .vehicleInfos[0].vehicleConfigurations[0].partNumber|=$PART_NUM1 |
        .vehicleInfos[0].vehicleConfigurations[0].publicKeyId|=$PUBLIC_KEY_ID |
        .vehicleInfos[0].vehicleConfigurations[0].keyType|=$KEY_TYPE |
        .vehicleInfos[0].vehicleConfigurations[0].softwareInfos[0].productNumber|=$PROD_NUMBER1 |
        .vehicleInfos[0].vehicleConfigurations[0].softwareInfos[0].subTargetId|=$SUBTARGET_ID1 |
        .vehicleInfos[0].vehicleConfigurations[1].elementId|=$ELEMENT_ID2 |
        .vehicleInfos[0].vehicleConfigurations[1].targetId|=$TARGET_ID2 |
        .vehicleInfos[0].vehicleConfigurations[1].partNumber|=$PART_NUM2 |
        .vehicleInfos[0].vehicleConfigurations[1].softwareInfos[0].productNumber|=$PROD_NUMBER2 |
        .vehicleInfos[0].vehicleConfigurations[1].softwareInfos[0].subTargetId|=$SUBTARGET_ID2 |
        .vehicleInfos[0].vehicleConfigurations[2].elementId|=$ELEMENT_ID3 |
        .vehicleInfos[0].vehicleConfigurations[2].targetId|=$TARGET_ID3 |
        .vehicleInfos[0].vehicleConfigurations[2].partNumber|=$PART_NUM3 |
        .vehicleInfos[0].vehicleConfigurations[2].softwareInfos[0].productNumber|=$PROD_NUMBER3 |
        .vehicleInfos[0].vehicleConfigurations[2].softwareInfos[0].subTargetId|=$SUBTARGET_ID3 |
        .vehicleInfos[0].vehicleConfigurations[3].elementId|=$ELEMENT_ID4 |
        .vehicleInfos[0].vehicleConfigurations[3].targetId|=$TARGET_ID4 |
        .vehicleInfos[0].vehicleConfigurations[3].partNumber|=$PART_NUM4 |
        .vehicleInfos[0].vehicleConfigurations[3].softwareInfos[0].productNumber|=$PROD_NUMBER4 |
        .vehicleInfos[0].vehicleConfigurations[3].softwareInfos[0].subTargetId|=$SUBTARGET_ID4' |
tus pdp sdpv3 rest create-vehicle --public-key $PUB_KEY --json -

###############
# store temporal values
###############
echo "create vehicle_information for next script..."
echo VIN=$VIN >vehicle_information
echo MAKECODE=$MAKECODE >>vehicle_information
echo PART_NUM1=$PART_NUM1 >>vehicle_information
echo PART_NUM2=$PART_NUM2 >>vehicle_information
echo PART_NUM3=$PART_NUM3 >>vehicle_information
echo PART_NUM4=$PART_NUM4 >>vehicle_information
echo ELEMENT_ID1=$ELEMENT_ID1 >>vehicle_information
echo ELEMENT_ID2=$ELEMENT_ID2 >>vehicle_information
echo ELEMENT_ID3=$ELEMENT_ID3 >>vehicle_information
echo ELEMENT_ID4=$ELEMENT_ID4 >>vehicle_information
echo TARGET_ID1=$TARGET_ID1 >>vehicle_information
echo TARGET_ID2=$TARGET_ID2 >>vehicle_information
echo TARGET_ID3=$TARGET_ID3 >>vehicle_information
echo TARGET_ID4=$TARGET_ID4 >>vehicle_information
echo VDESIGN_ID=$VDESIGN_ID >>vehicle_information
echo HWCOMP_ID1=$HWCOMP_ID1 >>vehicle_information
echo HWCOMP_ID2=$HWCOMP_ID2 >>vehicle_information
echo HWCOMP_ID3=$HWCOMP_ID3 >>vehicle_information
echo HWCOMP_ID4=$HWCOMP_ID4 >>vehicle_information
echo SUBTARGET_ID1=$SUBTARGET_ID1 >>vehicle_information
echo SUBTARGET_ID2=$SUBTARGET_ID2 >>vehicle_information
echo SUBTARGET_ID3=$SUBTARGET_ID3 >>vehicle_information
echo SUBTARGET_ID4=$SUBTARGET_ID4 >>vehicle_information
echo SYS_ID=$SYS_ID >>vehicle_information
echo SUBSYS_ID=$SUBSYS_ID >>vehicle_information
echo CREATE_VTYPE_ID=$CREATE_VTYPE_ID >>vehicle_information
echo ELEMENT_SWCONFIG_ID1=$ELEMENT_SWCONFIG_ID1 >>vehicle_information
echo ELEMENT_SWCONFIG_ID2=$ELEMENT_SWCONFIG_ID2 >>vehicle_information
echo ELEMENT_SWCONFIG_ID3=$ELEMENT_SWCONFIG_ID3 >>vehicle_information
echo ELEMENT_SWCONFIG_ID4=$ELEMENT_SWCONFIG_ID4 >>vehicle_information
echo PROD_NUMBER1=$PROD_NUMBER1 >>vehicle_information
echo PROD_NUMBER2=$PROD_NUMBER2 >>vehicle_information
echo PROD_NUMBER3=$PROD_NUMBER3 >>vehicle_information
echo PROD_NUMBER4=$PROD_NUMBER4 >>vehicle_information

# cleanup
rm -rf ./pubkey
