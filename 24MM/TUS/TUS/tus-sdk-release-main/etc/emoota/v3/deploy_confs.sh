#!/bin/bash
#TMC CONFIDENTIAL
#$TUSLibId$
#Copyright (C) 2022 TOYOTA MOTOR CORPORATION
#All Rights Reserved.

# dump configurations
set -e

if [ $# -ne 1 ]; then
    echo "USAGE: ./deploy_conf.sh <path to vehicle information output by init_vehicle.sh>"
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

if [ -z "$TARGET_HOSTNAME" ]; then
    read -p "TARGET_HOSTNAME (user@ip or name defined ~/.ssh/config can be used.):" TARGET_HOSTNAME
    export TARGET_HOSTNAME=$TARGET_HOSTNAME
fi

if [ -z "$TARGET_DEPLOY_DIR" ]; then
    read -p "TARGET_DEPLOY_DIR (deploy directory on target device): " TARGET_DEPLOY_DIR
    export TARGET_DEPLOY_DIR=$TARGET_DEPLOY_DIR
fi

###############
# Domain Configuration
###############
# Domain Configurations. In this tutorial, UO exists on the target device which Rpi1 domain exists.
# Please Update below configurations if needed.
export RPI1_DOMAIN_NAME="Rpi1"
export UO_HTTPD_IP="192.168.0.2"
export RPI1_GRPC_HOST="$UO_HTTPD_IP:50051"
export RPI2_DOMAIN_NAME="Rpi2"
export RPI2_GRPC_HOST="192.168.0.3:50051"
export RPI3_DOMAIN_NAME="Rpi3"
export RPI3_GRPC_HOST="192.168.0.4:50051"
export RPI4_DOMAIN_NAME="Rpi4"
export RPI4_GRPC_HOST="192.168.0.5:50051"

###############
# Deploy Configurations
###############
ECU_JSON=$(
    tus dtool provconf sdpv3 --print-json |
    jq --arg ELEMENT_ID1 "$ELEMENT_ID1" --arg TARGET_ID1 "$TARGET_ID1" --arg PROD_NUMBER1 "$PROD_NUMBER1" --arg SUBTARGET_ID1 "$SUBTARGET_ID1" --arg PART_NUMBER1 "$PART_NUM1" \
       --arg ELEMENT_ID2 "$ELEMENT_ID2" --arg TARGET_ID2 "$TARGET_ID2" --arg PROD_NUMBER2 "$PROD_NUMBER2" --arg SUBTARGET_ID2 "$SUBTARGET_ID2" --arg PART_NUMBER2 "$PART_NUM2" \
       --arg ELEMENT_ID3 "$ELEMENT_ID3" --arg TARGET_ID3 "$TARGET_ID3" --arg PROD_NUMBER3 "$PROD_NUMBER3" --arg SUBTARGET_ID3 "$SUBTARGET_ID3" --arg PART_NUMBER3 "$PART_NUM3" \
       --arg ELEMENT_ID4 "$ELEMENT_ID4" --arg TARGET_ID4 "$TARGET_ID4" --arg PROD_NUMBER4 "$PROD_NUMBER4" --arg SUBTARGET_ID4 "$SUBTARGET_ID4" --arg PART_NUMBER4 "$PART_NUM4" \
        '.ecus[0].target_id|=$TARGET_ID1 |
        .ecus[0].ecu_hardware_part_a_id|=$PART_NUMBER1 |
        .ecus[0].ecu_hardware_part_b_id|=$PART_NUMBER1 |
        .ecus[0].ecu_software_part_a_id[0].product_number|=$PROD_NUMBER1 |
        .ecus[0].ecu_software_part_b_id[0].product_number|=$PROD_NUMBER1 |
        .ecus[0].ecu_software_part_a_id[0].sub_target_id|=$SUBTARGET_ID1 |
        .ecus[0].ecu_software_part_b_id[0].sub_target_id|=$SUBTARGET_ID1 |
        .ecus[0].serial_num|=$ELEMENT_ID1 |
        .ecus[1].target_id|=$TARGET_ID2 |
        .ecus[1].ecu_hardware_part_a_id|=$PART_NUMBER2 |
        .ecus[1].ecu_hardware_part_b_id|=$PART_NUMBER2 |
        .ecus[1].ecu_software_part_a_id[0].product_number|=$PROD_NUMBER2 |
        .ecus[1].ecu_software_part_b_id[0].product_number|=$PROD_NUMBER2 |
        .ecus[1].ecu_software_part_a_id[0].sub_target_id|=$SUBTARGET_ID2 |
        .ecus[1].ecu_software_part_b_id[0].sub_target_id|=$SUBTARGET_ID2 |
        .ecus[1].serial_num|=$ELEMENT_ID2 |
        .ecus[1].dest_path|="./ecu" |
        .ecus[2].target_id|=$TARGET_ID3 |
        .ecus[2].ecu_hardware_part_a_id|=$PART_NUMBER3 |
        .ecus[2].ecu_hardware_part_b_id|=$PART_NUMBER3 |
        .ecus[2].ecu_software_part_a_id[0].product_number|=$PROD_NUMBER3 |
        .ecus[2].ecu_software_part_b_id[0].product_number|=$PROD_NUMBER3 |
        .ecus[2].ecu_software_part_a_id[0].sub_target_id|=$SUBTARGET_ID3 |
        .ecus[2].ecu_software_part_b_id[0].sub_target_id|=$SUBTARGET_ID3 |
        .ecus[2].serial_num|=$ELEMENT_ID3 |
        .ecus[2].dest_path|="./ecu" |
        .ecus[3].target_id|=$TARGET_ID4 |
        .ecus[3].ecu_hardware_part_a_id|=$PART_NUMBER4 |
        .ecus[3].ecu_hardware_part_b_id|=$PART_NUMBER4 |
        .ecus[3].ecu_software_part_a_id[0].product_number|=$PROD_NUMBER4 |
        .ecus[3].ecu_software_part_b_id[0].product_number|=$PROD_NUMBER4 |
        .ecus[3].ecu_software_part_a_id[0].sub_target_id|=$SUBTARGET_ID4 |
        .ecus[3].ecu_software_part_b_id[0].sub_target_id|=$SUBTARGET_ID4 |
        .ecus[3].serial_num|=$ELEMENT_ID4 |
        .ecus[3].dest_path|="./ecu"'
)

echo "deploy configurations to ${TARGET_HOSTNAME}:${TARGET_DEPLOY_DIR}"
tus dtool initconf $TARGET_DEPLOY_DIR/target --provider sdpv3
tus dtool rootconf --dest "$TARGET_DEPLOY_DIR"/target/uo --httpd-host "$UO_HTTPD_IP" ./provider/sdpv3/ ./domain
echo $ECU_JSON | tus dtool provconf sdpv3 --dest "$TARGET_DEPLOY_DIR"/target/uo/provider/sdpv3 --vin "$VIN" --vdesign-id "$VDESIGN_ID"
tus dtool domainconf root --dest "$TARGET_DEPLOY_DIR"/target/uo/domain "$RPI1_DOMAIN_NAME" ./rpi1 "$RPI2_DOMAIN_NAME" ./rpi2 "$RPI3_DOMAIN_NAME" ./rpi3 "$RPI4_DOMAIN_NAME" ./rpi4
tus dtool domainconf domain --dest "$TARGET_DEPLOY_DIR"/target/uo/domain --hostname "$RPI1_GRPC_HOST" --filename rpi1
tus dtool domainconf domain --dest "$TARGET_DEPLOY_DIR"/target/uo/domain --hostname "$RPI2_GRPC_HOST" --filename rpi2
tus dtool domainconf domain --dest "$TARGET_DEPLOY_DIR"/target/uo/domain --hostname "$RPI3_GRPC_HOST" --filename rpi3
tus dtool domainconf domain --dest "$TARGET_DEPLOY_DIR"/target/uo/domain --hostname "$RPI4_GRPC_HOST" --filename rpi4

###############
# DC Configuration
###############
dc_targets=(
    "tus-testbench-01"
    "tus-testbench-02"
    "tus-testbench-03"
    "tus-testbench-04"
)
for target in "${dc_targets[@]}" ; do
    TARGET_HOSTNAME_DC=${target} tus dtool dc rootconf ${target} --dest "${TARGET_DEPLOY_DIR}"/target/dc --grpc-listen 0.0.0.0:50051 --versions ./versions
done
