"""
TMC CONFIDENTIAL
$TUSLibId$
Copyright (C) 2022 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""

import argparse
import json
import logging
import os
import subprocess
from subprocess import PIPE
import sys
import tempfile
import ipaddress
from enum import Enum
from pathlib import Path

TARGET_HOSTNAME = "TARGET_HOSTNAME"
TARGET_HOSTNAME_DC = "TARGET_HOSTNAME_DC"
PATH_CONF_FILENAME = "config"
PROVIDER_CONF_FILENAME = "config"
DOMAIN_ROOT_CONF_FILENAME = "config"
DC_ROOT_CONF_FILENAME = "config"

ECU_JSON_FORMAT = """{
    "ecus": [
        {
            "dest_path": "./ecu",
            "target_id": "",
            "serial_num": "",
            "ecu_hardware_part_a_id": "",
            "ecu_hardware_part_b_id": "",
            "ecu_software_part_a_id": [{"sub_target_id": "", "product_number": ""}],
            "ecu_software_part_b_id": [{"sub_target_id": "", "product_number": ""}],
            "active_bank": "A",
            "rewrite_a_count": 0,
            "rewrite_b_count": 0
        }
    ]
}
"""


class _PrintJsonAction(argparse.Action):
    # custom action for handling --print-json option like --help
    # if this option is specified, print json set to default
    def __init__(self,
                 option_strings,
                 dest=argparse.SUPPRESS,
                 default=argparse.SUPPRESS,
                 help=None):
        super(_PrintJsonAction, self).__init__(
            option_strings=option_strings,
            dest=dest,
            default=default,
            nargs=0,
            help=help)

    def __call__(self, parser, namespace, values, option_string=None):
        print(f"{self.default}")
        parser.exit()


class Target(Enum):
    UO = 1
    DC = 2


logger = logging.getLogger(__name__)
if 1:
    logging.basicConfig(level=logging.INFO)


def create_common_parser() -> argparse.ArgumentParser:
    """create common parser independent of command

    Returns:
        argparse.ArgumentParser: [description]
    """
    parser = argparse.ArgumentParser(add_help=False)
    for names, opts in (
        [("-i",),
         dict(help=f"identity file",
              metavar="PATH",
              type=str)],
    ) + __construct_host_parser_options():
        parser.add_argument(*names, **opts)
    return parser


def __construct_host_parser_options(required: bool = False):
    return ([("--host",),
             dict(help=f"username@hostname, {TARGET_HOSTNAME} environment variable is defined if not specified",
                  required=required,
                  default=None,
                  type=str)],)


def __construct_conffile_options(required: bool = True, help_msg=f"destination folder to deploy"):
    return ([("--dest",),
             dict(help=help_msg,
                  required=required,
                  type=str)],)

def __deploy_file(hostname: str, dest: str, input: str, identity_file: str = None, target: Target = Target.UO):
    host = ""
    if hostname:
        host = hostname
    else:
        if target == Target.DC:
            host = os.getenv(TARGET_HOSTNAME_DC)
        else:
            host = os.getenv(TARGET_HOSTNAME)

    if host is None or host == "":
        logger.error("host is not specified.")
        sys.exit(1)

    if not os.path.exists(input):
        logger.error("input file does not exist.")
        sys.exit(1)

    if identity_file is not None and not os.path.exists(identity_file):
        logger.error("identity file does not exist.")
        sys.exit(1)

    dest_dir = os.path.dirname(dest)
    logger.info(f"deploying {host}:{dest}.")
    logger.debug(f"Will create directory {dest_dir} on {host} if it does not exist.")
    proc = None
    identity = f"-i {identity_file}" if identity_file else ""
    # create destination directory on target
    command = f"ssh {identity} {host} 'mkdir -p {dest_dir}'; " # ignore file exists error
    # copy file to target
    command += f"scp {identity} {input} {host}:{dest}"

    logger.debug(f"executing command {command}")
    proc = subprocess.run(command, shell=True, stdout=PIPE, stderr=PIPE)

    if proc.returncode == 0:
        logger.info("SUCCESSFUL to deploy")
    else:
        logger.error("FAILED to deploy")
        logger.error(f"error msg:{proc.stderr}")


def deploy_tup(args):
    __deploy_file(args.host, args.dest, args.file_path, args.i)


def deploy_under_dest_folder(hostname: str, parent_dest: str, dest: str, input: str, identity_file: str = None):
    __deploy_file(hostname, os.path.join(parent_dest, dest), input, identity_file)


def add_tup_parser(sub_parser):
    parser = sub_parser.add_parser("tup",
                                   help="deploy tup to target device",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in (
        [("--dest",),
         dict(help=f"destination file path",
              required=True,
              type=str)],
        [("file_path",),
         dict(help=f"tup file path to deploy",
              metavar="file-path",
              type=str)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_tup)


def deploy_rootconf(args):
    with tempfile.NamedTemporaryFile() as fp:
        template = f"""provider={args.prov_search_path}
domain={args.domain_search_path}
httpd_host={args.httpd_host}
"""
        fp.write(bytes(template, 'utf-8'))
        fp.flush()
        __deploy_file(args.host, os.path.join(
            args.dest, PATH_CONF_FILENAME), fp.name, args.i)


def add_rootconf_parser(sub_parser):
    parser = sub_parser.add_parser("rootconf",
                                   help="deploy root configuration ",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in __construct_conffile_options() + (
        [("prov_search_path",),
         dict(help=f"search path of provider configuration",
              type=str)],
        [("domain_search_path",),
         dict(help=f"search path of domain configuration",
              type=str)],
        [("--httpd-host",),
         dict(help=f"httpd configuration between UO and DC",
              default="localhost",
              type=str)]
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_rootconf)


def deploy_provlocal_conf(args):
    with tempfile.NamedTemporaryFile() as fp:
        template = f"""type=localfs
package_path={args.package_path}
"""
        fp.write(bytes(template, 'utf-8'))
        fp.flush()
        __deploy_file(args.host, os.path.join(
            args.dest, PROVIDER_CONF_FILENAME), fp.name, args.i)

    if args.deploy_package:
        # deploy package as well
        logger.info(f"deploying package {args.deploy_package} to {args.package_path}.")
        __deploy_file(args.host, args.package_path, args.deploy_package, args.i)

def add_prov_local_parser(sub_parser):
    parser = sub_parser.add_parser("local",
                                   help="deploy local provider configuration",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in __construct_conffile_options() + (
        [("package_path",),
            dict(help=f"tup path on target",
                 type=str)],
        [("--deploy_package",),
            dict(help=f"deploy specified package to package_path on target",
                 type=str,
                 default=None)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_provlocal_conf)


def deploy_prov_devhttpd_conf(args):
    with tempfile.NamedTemporaryFile() as fp:
        template = f"""type=devhttpd
httpd_server={args.server}
httpd_port={args.port}
httpd_url_base={args.httpd_url_base}
package_name={args.pkg_name}
version={args.pkg_version}
"""
        fp.write(bytes(template, 'utf-8'))
        fp.flush()
        __deploy_file(args.host, os.path.join(
            args.dest, PROVIDER_CONF_FILENAME), fp.name, args.i)


def add_prov_devhttpd_parser(sub_parser):
    parser = sub_parser.add_parser("devhttpd",
                                   help="""deploy devhttpd provider configuration.""",
                                   description="""Deploy devhttpd provider configuration.
                                   When using this provider, TUS runtime gets from server:port/httpd_url_base/pkg_version/pkg_name by HTTP request.
                                   After TUS runtime updates successfully, it increments pkg_version to search.
                                   """,
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in __construct_conffile_options() + (
        [("--server",),
            dict(help=f"httpd server address",
                 type=str,
                 required=True)],
        [("--port",),
         dict(help=f"httpd server port number",
              default=80,
              type=int)],
        [("--httpd-url-base",),
         dict(help=f"httpd url base to get tup files",
              default="/tus/package/",
              type=str)],
        [("--pkg-name",),
         dict(help=f"package name",
              type=str)],
        [("--pkg-version",),
         dict(help=f"package version",
              default=0,
              type=int)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_prov_devhttpd_conf)


def __check_id(expected_id, actual_id):
    ret_id = ""
    if actual_id:
        if actual_id != expected_id:
            logger.warning(
                f"id not matched input:{actual_id} on_server:{expected_id}")
        ret_id = actual_id
    else:
        logger.info(
            f"id not specified, use server value:{expected_id}")
        ret_id = expected_id

    return ret_id

def initialize_target(args):
    host = ""
    if args.host:
        host = args.host
    else:
        host = os.getenv(TARGET_HOSTNAME)

    if host is None or host == "":
        logger.error("host is not specified.")
        sys.exit(1)
    if args.i is not None and not os.path.exists(args.i):
        logger.error("identity file does not exist.")
        sys.exit(1)

    proc = None
    identity = f"-i {args.i}" if args.i else ""
    command = f'cd {args.dest} && \
                mkdir -p uo/domain'
    for provider in args.provider:
        if provider in ["sdpv3"]:
            command += f' uo/provider/{provider}/ecu'
        else:
            command += f' uo/provider/{provider}'
    logger.info(f"initialize {host}")
    logger.debug(f"executing command {command} on {host}")
    proc = subprocess.run(
        f"ssh {identity} {host} '{command}'", shell=True, stdout=PIPE, stderr=PIPE)
    if proc.returncode != 0:
        logger.error("FAILED to create directory")
        logger.error(f"error msg: {proc.stderr.decode('utf-8')}")
        return
    # send public key for PDP
    if provider in ["sdpv3"]:
        pub_key = "rootca.pem"
        command = f"scp {identity} {pub_key} {host}:{args.dest}/uo/provider/{provider}"
        logger.debug(f"executing command {command} on {host}")
        proc = subprocess.run(command, shell=True, stdout=PIPE, stderr=PIPE)
    if proc.returncode == 0:
        logger.info(f"initialized {host} successfully")
    else:
        logger.error("FAILED to initialize")
        logger.error(f"error msg: {proc.stderr.decode('utf-8')}")
        return

def add_initconf_parser(sub_parser):
    parser = sub_parser.add_parser("initconf",
                                   help="prepare directories for deploying configuration files",
                                   description="prepare directories for deploying configuration files.\
                                                This command will not overwrite if the directory already exists.",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in (
        [("dest",),
            dict(help="path to the directory which contains update orchestrator uo/uo",
                 metavar="PATH",
                 type=str)],
        [("--provider",),
            dict(help="PDP provider to use",
                 choices=["localfs", "devhttpd", "sdpv3"],
                 nargs="*",
                 default=["localfs"])],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=initialize_target)


def deploy_provsdpv3_conf(args):
    def check_software_infos(_subtarget_id, softid, active_bank, ecu_sw_a_id, ecu_sw_b_id):
        subtarget_id = ""
        ecu_sw_a_prod_num = ""
        ecu_sw_b_prod_num = ""

        if ecu_sw_a_id and "product_number" in ecu_sw_a_id.keys():  # use user input
            ecu_sw_a_prod_num = __check_id(softid, ecu_sw_a_id["product_number"])
        else:  # use server id since no user input was given
            ecu_sw_a_prod_num = __check_id(softid, None)
        if ecu_sw_b_id and "product_number" in ecu_sw_b_id.keys():  # use user input
            ecu_sw_b_prod_num = __check_id(softid, ecu_sw_b_id["product_number"])
        else:  # use server id since no user input was given
            ecu_sw_b_prod_num = __check_id(softid, None)

        if active_bank == 'A':
            if ecu_sw_a_id and "sub_target_id" in ecu_sw_a_id.keys():
                subtarget_id = ecu_sw_a_id["sub_target_id"]
            else:
                subtarget_id = ""
        elif active_bank == 'B':
            if ecu_sw_b_id and "sub_target_id" in ecu_sw_b_id.keys():
                subtarget_id = ecu_sw_b_id["sub_target_id"]
            else:
                subtarget_id = ""
        if _subtarget_id != subtarget_id:
            logger.warning(f"specified subtarget id not matched: input:{subtarget_id}, on_server:{_subtarget_id}")
        subtarget_id = _subtarget_id

        return subtarget_id, ecu_sw_a_prod_num, ecu_sw_b_prod_num

    def __generate_sdpv3_ecu_info(ecu_json, higher_config_dest, vehicle, current_conf):
        ECUS_KEY = "ecus"
        # path written in higher config file
        ret_file_path_in_config = []
        # path to pass scp
        ret_send_path = []
        ret_ecu_configs = []

        if ecu_json is None:
            raise ValueError("file path is not spcefieid.")

        with ecu_json as f:
            info = json.load(f)

        ecus = info.get(ECUS_KEY)
        if ecus is None:
            raise ValueError(f"invalid configuration file: {ECUS_KEY} not exist.")

        # parse each ECU config
        DEST_PATH_KEY = "dest_path"
        TARGET_ID_KEY = "target_id"
        SUBTARGET_ID_KEY = "sub_target_id"
        SERIAL_NUM_KEY = "serial_num"
        ECU_HW_A_KEY = "ecu_hardware_part_a_id"
        ECU_HW_B_KEY = "ecu_hardware_part_b_id"
        ECU_SW_A_KEY = "ecu_software_part_a_id"
        ECU_SW_B_KEY = "ecu_software_part_b_id"
        ACTIVE_BANK_KEY = "active_bank"
        REWRITE_A_COUNT_KEY = "rewrite_a_count"
        REWRITE_B_COUNT_KEY = "rewrite_b_count"

        for ecu in ecus:
            # mandatory parameter check
            dest_path = ecu.get(DEST_PATH_KEY)
            if dest_path is None:
                raise ValueError(f"{DEST_PATH_KEY} is not specified in ecu json.")
            target_id = ecu.get(TARGET_ID_KEY)
            if target_id is None:
                raise ValueError(f"{TARGET_ID_KEY} is not specified in ecu json.")
            serial_num = ecu.get(SERIAL_NUM_KEY)
            if not serial_num:
                raise ValueError(f"{SERIAL_NUM_KEY} is not specified in ecu json.")

            written_in_config_file_path = os.path.join(
                dest_path, target_id)
            ret_file_path_in_config.append(written_in_config_file_path)
            if os.path.isabs(dest_path):
                ret_send_path.append(written_in_config_file_path)
            else:
                # dest_path is expected to be an relative path from config.
                ret_send_path.append(os.path.join(
                    higher_config_dest, written_in_config_file_path))

            # check with server defined value (if not specified use value)
            ecu_hw_a_id = ecu.get(ECU_HW_A_KEY)
            ecu_hw_b_id = ecu.get(ECU_HW_B_KEY)
            ecu_sw_a_ids = ecu.get(ECU_SW_A_KEY)
            ecu_sw_b_ids = ecu.get(ECU_SW_B_KEY)
            active_bank = ecu.get(ACTIVE_BANK_KEY, 'A')
            if active_bank not in ['A', 'B']:
                raise ValueError("Bank shall be A or B.")
            rewrite_a_count = ecu.get(REWRITE_A_COUNT_KEY, 0)
            rewrite_b_count = ecu.get(REWRITE_B_COUNT_KEY, 0)

            content = {
                    SERIAL_NUM_KEY: serial_num,
                    ACTIVE_BANK_KEY: active_bank,
                    REWRITE_A_COUNT_KEY: rewrite_a_count,
                    REWRITE_B_COUNT_KEY: rewrite_b_count
            }

            # prioritize current configuration
            if current_conf:
                hwcomps = []
                sysconfs = current_conf.get("systemConfigurations")
                for sysconf in sysconfs:
                    hwcomps.extend(sysconf.get("hardwareComponents"))
                is_exist = False
                for hwcomp in hwcomps:
                    if target_id == hwcomp.get("targetId"):
                        is_exist = True
                        # check active bank
                        bank = hwcomp.get("bank")
                        if active_bank != bank:
                            logger.warning(
                                f"active bank not matched input:{active_bank} on_server:{bank}")
                        # check partNumber
                        hwid = hwcomp.get("partNumber")
                        if active_bank == 'A':
                            ecu_hw_a_id = __check_id(hwid, ecu_hw_a_id)
                        elif active_bank == 'B':
                            ecu_hw_b_id = __check_id(hwid, ecu_hw_b_id)

                        content[ECU_HW_A_KEY] = ecu_hw_a_id
                        content[ECU_HW_B_KEY] = ecu_hw_b_id
                        for (i, soft) in enumerate(hwcomp.get("softwares")):
                            # ONLY current_config uses softwareId not productNumber
                            # check softwareId and subTargetId
                            softid = soft.get("softwareId")
                            _subtarget_id = soft.get("subTargetId")
                            actual_ecu_sw_a_id = ecu_sw_a_ids[i] if len(ecu_sw_a_ids) < i else ""
                            actual_ecu_sw_b_id = ecu_sw_b_ids[i] if len(ecu_sw_b_ids) < i else ""
                            subtarget_id, ecu_sw_a_prod_num, ecu_sw_b_prod_num = check_software_infos(_subtarget_id, softid, active_bank, actual_ecu_sw_a_id, actual_ecu_sw_b_id)

                            content[ECU_SW_A_KEY + "/" + subtarget_id] = ecu_sw_a_prod_num
                            content[ECU_SW_B_KEY + "/" + subtarget_id] = ecu_sw_b_prod_num
                if not is_exist:
                    logger.warning(
                        f"specified ecu does not exist. target_id:{target_id} serial_num:{serial_num}")
            else:
                # only use vehicle configuration information if not get current configuration
                vehicle_confs = vehicle.get("vehicleConfigurations")
                is_exist = False
                for vehicle_conf in vehicle_confs:
                    is_exist = True
                    element_id = vehicle_conf.get("elementId")
                    if serial_num == element_id:
                        # check partNumber
                        hwid = vehicle_conf.get("partNumber")
                        if active_bank == 'A':
                            ecu_hw_a_id = __check_id(hwid, ecu_hw_a_id)
                        elif active_bank == 'B':
                            ecu_hw_b_id = __check_id(hwid, ecu_hw_b_id)

                        content[ECU_HW_A_KEY] = ecu_hw_a_id
                        content[ECU_HW_B_KEY] = ecu_hw_b_id

                        for (i, soft) in enumerate(vehicle_conf.get("softwareInfos")):
                            # check productNumber and subTargetId
                            softid = soft.get("productNumber")
                            _subtarget_id = soft.get("subTargetId")
                            actual_ecu_sw_a_id = ecu_sw_a_ids[i] if len(ecu_sw_a_ids) > i else ""
                            actual_ecu_sw_b_id = ecu_sw_b_ids[i] if len(ecu_sw_b_ids) > i else ""
                            subtarget_id, ecu_sw_a_prod_num, ecu_sw_b_prod_num = check_software_infos(_subtarget_id, softid, active_bank, actual_ecu_sw_a_id, actual_ecu_sw_b_id)

                            content[ECU_SW_A_KEY + "/" + subtarget_id] = ecu_sw_a_prod_num
                            content[ECU_SW_B_KEY + "/" + subtarget_id] = ecu_sw_b_prod_num
                if not is_exist:
                    logger.warning(
                        f"specified ecu does not exist. target_id:{target_id} serial_num:{serial_num}")

            ret_ecu_configs.append(content)

        logger.debug(f"current config : {current_conf}")
        logger.debug(f"vehicle: {vehicle}")
        logger.debug(f"ecu_config: {ret_ecu_configs}")
        logger.debug(f"ret_send_path: {ret_send_path}")
        logger.debug(f"ret_file_path_in_config: {ret_file_path_in_config}")
        return ret_ecu_configs, ret_send_path, ret_file_path_in_config

    def __fill_sdpv3_info(output, rxswins):
        # collect vehicle info
        proc = subprocess.run(
            f"python3 -m sdpv3.main rest get-vehicle {vin} --print-json", shell=True, stdout=PIPE, stderr=PIPE)
        if proc.returncode == 0:
            vehicle = json.loads(proc.stdout.decode('utf8'))
            if vehicle == {}:
                raise ValueError(f"vehile which has {vin} is not registerred.")
        else:
            logger.error(f"error msg:{proc.stderr}")
            exit(1)

        prim_element_id = vehicle.get("primaryElementId")
        if output[DCM_SERIAL_NUM_KEY] is None:
            output[DCM_SERIAL_NUM_KEY] = prim_element_id
        if output[DCM_SERIAL_NUM_KEY] != prim_element_id:
            logger.warning(f"As a primary element id, specified {output[DCM_SERIAL_NUM_KEY]} is used, but server registerrerd one is {prim_element_id}")

        # collect from vehicle design (if not initial vehicle design is specified use previous values.)
        proc = subprocess.run(
            f"python3 -m sdpv3.main rest get-vdesign {args.vdesign_id} --print-json", shell=True, stdout=PIPE, stderr=PIPE)
        if proc.returncode == 0:
            ret = json.loads(proc.stdout)
            subsystems = ret["subsystems"]
            for subsystem in subsystems:
                logger.debug(f"subsystem => {subsystem}")
                ecus = subsystem["subsystemConfiguration"]["components"]
                for ecu in ecus:
                    for i in range(len(vehicle["vehicleConfigurations"])):
                        if ecu["hardware"]["partNumber"] == vehicle["vehicleConfigurations"][i]["partNumber"]:
                            vehicle["vehicleConfigurations"][i]["softwareInfos"] = ecu["softwareInfos"]
                            break
                if not rxswins:
                    subsystem_conf = subsystem.get("subsystemConfiguration")
                    prev_rxswins = subsystem_conf.get("previousRxswins")
                    if prev_rxswins:
                        for i, rxswin in enumerate(prev_rxswins):
                            rxswins[f"data{i}"] = rxswin
                    else:
                        _rxswins = subsystem_conf.get("rxswins")
                        for i, rxswin in enumerate(_rxswins):
                            rxswins[f"data{i}"] = rxswin

        # collect current configuraton (may empty if not communicated vehicle and server)
        proc = subprocess.run(
            f"python3 -m sdpv3.main rest get-vin-currconf {vin} --print-json", shell=True, stdout=PIPE, stderr=PIPE)
        if proc.returncode == 0:
            ret = json.loads(proc.stdout)
            if ret:
                currconf = ret
            else:
                logger.info("no current configuration")
                currconf = None
        else:
            logger.error(f"error msg:{proc.stderr}")
            exit(1)

        # overwrite campaign id with the latest configuration
        if output[CAMPAIGN_ID_KEY] is None:
            output[CAMPAIGN_ID_KEY] = "unknown"
            if currconf:
                campaign_info = currconf.get("campaignInfo")
                if campaign_info:
                    logger.info(f"use campain id {campaign_info['campaignId']}")
                    output[CAMPAIGN_ID_KEY] = campaign_info["campaignId"]

        return vehicle, currconf

    vin = args.vin
    # validation
    if len(vin) != 17:
        raise ValueError(f"vin length should be 17 length.")
    if args.rootca_input is not None and not os.path.exists(args.rootca_input):
        raise ValueError(f"specified rootca input does not exist.")

    TYPE_KEY = "type"
    CDN_ADDR_KEY = "cdn_url"
    CDN_PORT_KEY = "cdn_port"
    GW_ADDR_KEY = "sdp_url"
    GW_PORT_KEY = "sdp_port"
    VIN_KEY = "vin"
    DCM_SERIAL_NUM_KEY = "dcm_serial_num"
    CAMPAIGN_ID_KEY = "campaign_id"
    LAST_COMPLETED_ID_KEY = "last_completed_uid"
    ROOT_CA_KEY = "rootca"
    RXSWIN_KEY = "rxswin"
    ECU_KEY_PREFIX = "ecu/"

    output = {}
    output[TYPE_KEY] = "sdpv3"
    output[CDN_ADDR_KEY] = args.server_host
    output[CDN_PORT_KEY] = args.server_port
    output[GW_ADDR_KEY] = args.gw_host
    output[GW_PORT_KEY] = args.gw_port
    output[VIN_KEY] = vin

    output[DCM_SERIAL_NUM_KEY] = args.prim_elementid  # filled by factoryfeed response, called elementId on server

    output[CAMPAIGN_ID_KEY] = args.last_campaignid  # filled by current-config.

    # server may not have below info
    output[LAST_COMPLETED_ID_KEY] = args.last_completionid

    output[ROOT_CA_KEY] = args.rootca_dest
    output[RXSWIN_KEY] = args.rxswin_dest  # filled by vehicle-degin previousRxswin (if not updated vehicle design, rxswins is fine)

    rxswins = {}
    if args.rxswins is not None:
        for i, rxswin in enumerate(args.rxswins):
            rxswins[f"data{i}"] = rxswin

    # confirm server setting (by default use user specified value.)
    vehicle_res, currconf_res = __fill_sdpv3_info(output, rxswins)
    ecu_configs, send_path, file_paths_in_config = __generate_sdpv3_ecu_info(args.ecu_json, args.dest, vehicle_res, currconf_res)
    # set each ecu config files
    for file_path in file_paths_in_config:
        target_id = os.path.basename(file_path)
        output[ECU_KEY_PREFIX + target_id] = file_path

    logger.info("deploy config file")
    with tempfile.NamedTemporaryFile() as fp:
        data = ""
        for k, v in output.items():
            data += f"{k}={v}\n"

        fp.write(bytes(data, 'utf-8'))
        fp.flush()
        __deploy_file(args.host, os.path.join(
            args.dest, PROVIDER_CONF_FILENAME), fp.name, args.i)

    # send ecu config files.
    for i, config in enumerate(ecu_configs):
        with tempfile.NamedTemporaryFile() as fp:
            data = ""
            for k, v in config.items():
                data += f"{k}={v}\n"
            fp.write(bytes(data, 'utf-8'))
            fp.flush()
            __deploy_file(args.host, send_path[i], fp.name, args.i)

    if rxswins:
        logger.info("deploy rxswins file")
        with tempfile.NamedTemporaryFile() as fp:
            data = ""
            for k, v in rxswins.items():
                data += f"{k}={v}\n"

            fp.write(bytes(data, 'utf-8'))
            fp.flush()
            deploy_under_dest_folder(args.host, args.dest, args.rxswin_dest, fp.name, args.i)

    if args.rootca_input:
        logger.info("deploy rootca file")
        deploy_under_dest_folder(args.host, args.dest, args.rootca_dest, args.rootca_input, args.i)


def add_prov_sdpv3_parser(sub_parser):
    sdpv3_name = "sdpv3"
    desc_msg = f"""
deploy {sdpv3_name} provider three configuration files ({sdpv3_name} configuration, rxswins and root.ca).
It requires that environment variable SDP_USERNAME and SDP_PASSWORD to access SDP resources.
"""
    parser = sub_parser.add_parser(
        f"{sdpv3_name}", help=f"deploy {sdpv3_name} provider configuration", description=desc_msg,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in __construct_conffile_options(help_msg=f"folder path to deploy {sdpv3_name} configuration") + (
        [("--server-host",),
         dict(help=f"{sdpv3_name} server hostname",
              default=f"https://t-poc-r3-ap-northeast-1-cdn-unsigned.t-poc-r3.com",
              type=str)],
        [("--server-port",),
         dict(help=f"{sdpv3_name} server port number",
              default=443,
              type=int)],
        [("--gw-host",),
         dict(help=f"{sdpv3_name} gateway hostname",
              default=f"https://t-poc-r3-ap-northeast-1-gw-mob.t-poc-r3.com",
              type=str)],
        [("--gw-port",),
         dict(help=f"{sdpv3_name} gateway port number",
              default=443,
              type=int)],
        [("--vin",),
         dict(help=f"VIN",
              required=True,
              type=str)],
        [("--vdesign-id",),
         dict(help=f"vehicle design id which is registered for update",
              required=True,
              type=str)],
        [("--prim-elementid",),
         dict(help=f"primary ecu serial number, if not set automatically collected from server",
              default=None,
              type=str)],
        [("--last-campaignid",),
         dict(help=f"last campaign id, if not set automatically collected from server",
              default=None,
              type=str)],
        [("--last-completionid",),
         dict(help=f"last completed uid",
              default=0,
              type=int)],
        [("--rxswin-dest",),
         dict(help=f"rxswin configuration relative file path from config file on target",
              default="./rxswin",
              type=str)],
        [("--rxswins",),
         dict(help=f"rxswin configuration, if not set automatically collected from server",
              default=None,
              nargs="*",
              type=str)],
        [("--rootca-dest",),
         dict(help=f"rootca relative file path from config file on target",
              default="./rootca.pem",
              type=str)],
        [("--rootca-input",),
         dict(help=f"rootca file to pass, if not set not update root.ca",
              default=None,
              type=str)],
        [("--ecu-json",),
         dict(help="ecu information json path or stdin",
              default="-",
              type=argparse.FileType("r"),
              )],
        [("--print-json",),
         dict(help="print ecu json format",
              default=ECU_JSON_FORMAT,
              action=_PrintJsonAction
              )],

    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_provsdpv3_conf)


def deploy_domain_root_conf(args):
    if len(args.domain_info) % 2 != 0:
        raise ValueError(
            "domain-info should be pair lists of domain name and destination file path.")
    with tempfile.NamedTemporaryFile() as fp:
        template = ""
        for i in range(0, len(args.domain_info), 2):
            template += f"{args.domain_info[i]}={args.domain_info[i+1]}\n"

        fp.write(bytes(template, 'utf-8'))
        fp.flush()
        __deploy_file(args.host, os.path.join(
            args.dest, DOMAIN_ROOT_CONF_FILENAME), fp.name, args.i)
    pass


def add_domain_root_parser(sub_parser):
    parser = sub_parser.add_parser("root",
                                   help="deploy root configuration for managing each domain configuration",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in __construct_conffile_options() + (
        [("domain_info",),
            dict(help=f"""
 Each domain configuration file information list, pairs of domain name and file path of domain configuration file like "tus dtool domainconf Ubuntu ./ubuntu Raspi ./raspi".
 Each domain name should be the same as passed in TUP generator.
            """,
                 nargs="+",
                 metavar="domain-info",
                 type=str)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_domain_root_conf)


def deploy_domain_domain_conf(args):
    with tempfile.NamedTemporaryFile() as fp:
        template = f"""{args.protocol}={args.hostname}"""
        fp.write(bytes(template, 'utf-8'))
        fp.flush()
        __deploy_file(args.host, os.path.join(
            args.dest, args.filename), fp.name, args.i)


def add_domain_domain_parser(sub_parser):
    parser = sub_parser.add_parser("domain",
                                   help="deploy each domain configuration",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    # folder path and file name parameters are split for consistency usage of --dest
    for names, opts in __construct_conffile_options(help_msg="folder path of domain configuration file") + (
        [("--protocol",),
            dict(help=f"protocol to connect from UO",
                 default=f"grpc",
                 type=str)],
        [("--hostname",),
            dict(help=f"hostname to connect from UO",
                 default=f"localhost:50051",
                 type=str)],
        [("--filename",),
            dict(help=f"filename of domain configuration. It shall be defined in domain root configuration file",
                 required=True,
                 type=str)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_domain_domain_conf)


def add_domainconf_parser(sub_parser):
    parser = sub_parser.add_parser("domainconf",
                                   help="deploy domain configuration ")
    prov_sub_parser = parser.add_subparsers(help="domain configuration type",
                                            dest="domain_cmd")
    add_domain_root_parser(prov_sub_parser)
    add_domain_domain_parser(prov_sub_parser)


def add_provconf_parser(sub_parser):
    parser = sub_parser.add_parser("provconf",
                                   help="deploy provider configuration ")
    prov_sub_parser = parser.add_subparsers(help="provider type",
                                            dest="prov_cmd")
    add_prov_local_parser(prov_sub_parser)
    add_prov_sdpv3_parser(prov_sub_parser)
    add_prov_devhttpd_parser(prov_sub_parser)

def deploy_dc_root_conf(args):
    # validate grpc listen IP address and port number
    try:
        listen_ip, listen_port = args.grpc_listen.split(":")
        ipaddress.ip_address(listen_ip)
    except ValueError:
        raise ValueError('Given interface "{}" is invalid.'.format(args.grpc_listen))
    if not (listen_port.isdigit() and (0 <= int(listen_port) <= 65535)):
        raise ValueError('Given interface "{}" is invalid.'.format(args.grpc_listen))

    # validate args.versions is within args.dest
    if Path(args.dest).resolve() not in Path(os.path.join(args.dest, args.versions)).resolve().parents:
        raise ValueError('Versions path {} should be within {}'.format(args.versions, args.dest))
    with tempfile.NamedTemporaryFile() as fp:
        data = f"id={args.id}\n"
        data += f"grpc_listen={args.grpc_listen}\n"
        data += f"versions={args.versions}\n"
        fp.write(bytes(data, 'utf-8'))
        fp.flush()
        __deploy_file(args.host, os.path.join(args.dest, DC_ROOT_CONF_FILENAME), fp.name, args.i, target=Target.DC)
    return

def add_dc_root_parser(sub_parser):
    parser = sub_parser.add_parser("rootconf",
                                   help="deploy root configuration for managing domain controller",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in __construct_conffile_options() + (
        [("id",),
            dict(help=f"ID of the target domain controller",
                 type=str)],
        [("--grpc-listen",),
            dict(help=f"interface for gRPC server to listen to",
                 type=str,
                 default="0.0.0.0:50051")],
        [("--versions",),
            dict(help="path to directory which contains DC's version information",
                 default="./versions",
                 metavar="PATH")],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=deploy_dc_root_conf)

def add_dc_parser(sub_parser):
    parser = sub_parser.add_parser("dc",
                                   help="deploy files for domain controller ")
    dc_sub_parser = parser.add_subparsers(help="config type", dest="dc_cmd")

    add_dc_root_parser(dc_sub_parser)

def create_parsers(needsCommonParser: bool = True) -> argparse.ArgumentParser:
    root_parser = argparse.ArgumentParser(
        prog="tus dtool",
        description="""
    TUS-SDK deployment tool for targets.
""",
        parents=([create_common_parser()] if needsCommonParser else []))

    sub_parsers = root_parser.add_subparsers(help="help",
                                             dest="dtool_cmd")
    add_initconf_parser(sub_parsers)
    add_tup_parser(sub_parsers)
    add_rootconf_parser(sub_parsers)
    add_domainconf_parser(sub_parsers)
    add_provconf_parser(sub_parsers)
    add_dc_parser(sub_parsers)

    return root_parser


def main(prog, argv):
    parser = create_parsers()
    args = parser.parse_args(argv)
    if args.dtool_cmd is None:
        parser.print_help()
        exit(0)
    # retrieve subparsers from parser
    subparsers_action = [
        action for action in parser._actions
        if isinstance(action, argparse._SubParsersAction)][0]
    if "domain_cmd" in args.__dict__.keys() and args.domain_cmd is None:
        # print help message from subparser
        subparsers_action.choices["domainconf"].print_help()
        exit(0)
    if "prov_cmd" in args.__dict__.keys() and args.prov_cmd is None:
        # print help message from subparser
        subparsers_action.choices["provconf"].print_help()
        exit(0)
    if "dc_cmd" in args.__dict__.keys() and args.dc_cmd is None:
        # print help message from subparser
        subparsers_action.choices["dc"].print_help()
        exit(0)
    args.func(args)


if __name__ == "__main__":
    main(prog=sys.argv[0], argv=sys.argv[1:])
