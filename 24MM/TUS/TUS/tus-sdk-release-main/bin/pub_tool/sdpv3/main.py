from .constants import B2BPORTAL
from .cli_parsers import CLIParsersV3
import argparse
import logging
import sys

from common.main import (
    add_approve_campaign_parser, add_approve_change_event_parser, add_approve_chevent_package_parser, add_approve_vdesign_parser, add_cancel_campaign_parser, add_create_campaign_parser,
    add_create_vtype_parser,
    add_submit_campaign_parser, add_submit_change_event_parser, add_update_chevent_package_parser
)

cli_parsers = CLIParsersV3()

logger = logging.getLogger(__name__)


def add_rest_parser(subParsers: argparse._SubParsersAction):
    rest_parser = subParsers.add_parser("rest",
                                        help="wraps some REST APIs",)
    rest_sub_parser = rest_parser.add_subparsers(help="help",
                                                 dest="rest_cmd")
    # vehicle type
    add_create_vtype_parser(rest_sub_parser)
    cli_parsers.add_search_vtype_parser(rest_sub_parser)
    cli_parsers.add_get_vtype_parser(rest_sub_parser)
    cli_parsers.add_delete_vtype_parser(rest_sub_parser)

    # vehicle type region
    cli_parsers.add_create_vtype_region_parser(rest_sub_parser)
    cli_parsers.add_search_vtype_region_parser(rest_sub_parser)
    cli_parsers.add_get_vtype_region_parser(rest_sub_parser)
    cli_parsers.add_delete_vtype_region_parser(rest_sub_parser)

    # subsystem
    cli_parsers.add_create_subsystem_parser(rest_sub_parser)
    cli_parsers.add_get_subsystem_parser(rest_sub_parser)
    cli_parsers.add_delete_subsystem_parser(rest_sub_parser)

    # hwcomponent
    cli_parsers.add_create_hwcomp_parser(rest_sub_parser)
    cli_parsers.add_search_hwcomp_parser(rest_sub_parser)
    cli_parsers.add_get_hwcomp_parser(rest_sub_parser)
    # no delete API for hwcomp

    # subsystemconf
    cli_parsers.add_create_subsystemconf_parser(rest_sub_parser)
    cli_parsers.add_update_subsystemconf_parser(rest_sub_parser)
    # no delete API for subsystem conf

    # vehicle design
    cli_parsers.add_create_vdesign_parser(rest_sub_parser)
    cli_parsers.add_get_vdesign_parser(rest_sub_parser)
    add_approve_vdesign_parser(rest_sub_parser)
    cli_parsers.add_delete_vdesign_parser(rest_sub_parser)

    # factory feed
    cli_parsers.add_create_vehicle_parser(rest_sub_parser)
    cli_parsers.add_search_vehicle_parser(rest_sub_parser)
    cli_parsers.add_get_vehicle_parser(rest_sub_parser)
    cli_parsers.add_delete_vehicle_parser(rest_sub_parser)

    # change event
    cli_parsers.add_create_change_event_parser(rest_sub_parser)
    cli_parsers.add_get_change_event_parser(rest_sub_parser)
    cli_parsers.add_update_change_event_parser(rest_sub_parser)
    add_submit_change_event_parser(rest_sub_parser)
    add_approve_change_event_parser(rest_sub_parser)
    cli_parsers.add_delete_change_event_parser(rest_sub_parser)

    # subsysdiff
    cli_parsers.add_create_subsysdiff_parser(rest_sub_parser)

    # change event package
    cli_parsers.add_create_chevent_package_parser(rest_sub_parser)
    add_update_chevent_package_parser(rest_sub_parser)
    add_approve_chevent_package_parser(rest_sub_parser)
    cli_parsers.add_get_chevent_package_parser(rest_sub_parser)
    cli_parsers.add_delete_change_event_package_parser(rest_sub_parser)

    # campaign
    add_create_campaign_parser(rest_sub_parser)
    cli_parsers.add_update_campaign_parser(rest_sub_parser)
    cli_parsers.add_get_campaign_parser(rest_sub_parser)
    add_submit_campaign_parser(rest_sub_parser)
    add_approve_campaign_parser(rest_sub_parser)
    add_cancel_campaign_parser(rest_sub_parser)
    cli_parsers.add_delete_campaign_parser(rest_sub_parser)

    # help desk related ops
    cli_parsers.add_get_vehicle_sysevent_parser(rest_sub_parser)
    cli_parsers.add_get_vehicle_config_parser(rest_sub_parser)
    # not need to support yet
    # add_get_vehicle_pendcampaign_parser(rest_sub_parser)
    cli_parsers.add_get_vehicle_histupdate_parser(rest_sub_parser)
    cli_parsers.add_get_vehicle_histconfig_parser(rest_sub_parser)


def add_ctrl_parser(subParsers: argparse._SubParsersAction):
    cli_parsers.add_pkg_upload_parser(subParsers)
    cli_parsers.add_get_package_processing_status_parser(subParsers)
    cli_parsers.add_get_vehicle_config_info_parser(subParsers)
    cli_parsers.add_approve_package_parser(subParsers)
    cli_parsers.add_pkg_download_parser(subParsers)


def create_common_parser() -> argparse.ArgumentParser:
    """create common parser independent of command

    Returns:
        argparse.ArgumentParser: [description]
    """
    parser = argparse.ArgumentParser(add_help=False)
    for names, opts in (
            [("--endpoint",),
             dict(help=f"base URL of EMOOTA SDPv3. default='{B2BPORTAL}'",
                  default=B2BPORTAL,
                  type=str)],
            [("--username",),
             dict(help="username of EMOOTA SDPv3. SDP_USERNAME environment variable is used if not specified."
                  " SDP_PASSWD environment variable should be defined.",
                  default=None,
                  type=str)]):
        parser.add_argument(*names, **opts)
    return parser


def create_parsers(needsCommonParser: bool = True) -> argparse.ArgumentParser:
    root_parser = argparse.ArgumentParser(
        prog="tus pdp sdpv3",
        description="""
        EMOOTA SDPv3 operator
        To set username and password of EMOOTA SDPv3, please define SDP_USERNAME and SDP_PASSWD environment variables.
""",
        parents=([create_common_parser()] if needsCommonParser else []))

    sub_parsers = root_parser.add_subparsers(help="help", dest="sdpv3_cmd")
    add_rest_parser(sub_parsers)
    add_ctrl_parser(sub_parsers)

    return root_parser


def main(prog, argv):
    parser = create_parsers()
    args = parser.parse_args(argv)
    if args.sdpv3_cmd is None:
        parser.print_help()
        exit(0)
    if "rest_cmd" in args.__dict__.keys() and args.rest_cmd is None:
        # retrieve subparsers from parser
        subparsers_action = [
            action for action in parser._actions
            if isinstance(action, argparse._SubParsersAction)][0]
        # print help message from subparser
        subparsers_action.choices["rest"].print_help()
        exit(0)
    args.func(args)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    logger.debug("call v3 client")
    main(prog=sys.argv[0], argv=sys.argv[1:])
