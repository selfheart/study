#!/usr/bin/python3
"""
TMC CONFIDENTIAL
$TUSLibId$
Copyright (C) 2022 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""

import argparse
import datetime
import logging
import logging.config
import os
import json
from typing import Dict

from .ctx import CTX
from .constants import DEFAULT_REGION_CODE, DEFAULT_YEAR_CODE, SDP_PASSWD, SDP_USERNAME

logger = logging.getLogger(__name__)

rxswins_parser_options = ([("--rxswins",),
                           dict(help=f"RXSWINs",
                                default="",
                                nargs="*",
                                type=str)],)


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

def construct_out_parser_options(help_msg: str = "output file path", required: bool = False):
    return ([("--out",),
             dict(help=help_msg,
                  default=None,
                  required=required,
                  metavar="PATH",
                  type=str)],)

def add_id_parser(sub_parser: argparse._SubParsersAction, cmd_name: str, help_cmd_msg: str, help_id_field_msg: str, default_func, meta_var: str = "id"):
    parser = sub_parser.add_parser(cmd_name,
                                   help=help_cmd_msg)
    for names, opts in (
        [("id",),
         dict(help=help_id_field_msg,
              metavar=meta_var,
              type=str)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=default_func)

def __add_approve_parser(sub_parser: argparse._SubParsersAction, cmd_name: str, help_cmd_msg: str, help_id_field_msg: str, default_func):
    add_id_parser(sub_parser, cmd_name, help_cmd_msg,
                    help_id_field_msg, default_func)

def __add_submit_parser(sub_parser: argparse._SubParsersAction, cmd_name: str, help_cmd_msg: str, help_id_field_msg: str, default_func):
    add_id_parser(sub_parser, cmd_name, help_cmd_msg,
                    help_id_field_msg, default_func)

def __add_cancel_parser(sub_parser: argparse._SubParsersAction, cmd_name: str, help_cmd_msg: str, help_id_field_msg: str, default_func):
    add_id_parser(sub_parser, cmd_name, help_cmd_msg,
                    help_id_field_msg, default_func)

def __output_response(ret: Dict):
    print(json.dumps(ret))

def __init_context(args):
    """
        init context object to call apis for SDP
    Args:
        args (_type_): _description_
    """
    username = args.username
    if username is None:
        username = os.getenv(SDP_USERNAME)
        if username is None:
            raise ValueError(
                f"Please pass --username or define {SDP_USERNAME} environment variable.")

    passwd = os.getenv(SDP_PASSWD)
    if passwd is None:
        raise ValueError(f"Please define {SDP_PASSWD} environment variable.")
    return CTX(args.endpoint, username, passwd)

def create_vtype(args: argparse.Namespace):
    logger.debug(f"{create_vtype.__name__} is called")
    context = __init_context(args)
    # if not created, raise error internally
    model_code = args.model_code if args.model_code else args.makecode
    trim_code = args.trim_code if args.trim_code else args.makecode
    context.vsms_admin__vehicle_type__create(
        args.makecode, model_code, trim_code, args.year_code, args.region)
    # call create vehicle type and search vtype to print vehicle type id
    ret = context.vsms_admin__vehicle_type__get(
        maker=args.makecode, model=model_code, trim=trim_code, year=args.year_code, exact=True)
    __output_response(ret)

def add_create_vtype_parser(sub_parser: argparse._SubParsersAction):
    parser = sub_parser.add_parser("create-vtype",
                                   help="create vehicle type",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in (
        [("makecode",),
         dict(help=f"make code",
              type=str)],
        [("--model-code",),
         dict(help=f"model code, if not set make code is used as a default value",
              default=None,
              type=str)],
        [("--trim-code",),
         dict(help=f"trim code if not set make code is used as a default value",
              default=None,
              type=str)],
        [("--year-code",),
         dict(help=f"year code",
              default=DEFAULT_YEAR_CODE,
              type=str)],
        [("--region",),
         dict(help=f"region",
              default=DEFAULT_REGION_CODE,
              type=str)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=create_vtype)

def approve_vdesign(args: argparse.Namespace):
    logger.debug(f"{approve_vdesign.__name__} is called")
    context = __init_context(args)
    ret = context.vsms_admin__vehicle_design__approve(args.id)
    __output_response(ret)
    return

def add_approve_vdesign_parser(sub_parser: argparse._SubParsersAction):
    __add_approve_parser(sub_parser, "approve-vdesign",
                         "approve vehicle design", "vehicle design id", approve_vdesign)

def approve_change_event(args: argparse.Namespace):
    logger.debug(f"{approve_change_event.__name__} is called")
    context = __init_context(args)
    ret = context.vsms_admin__change_event__approve(args.id)
    __output_response(ret)
    return

def add_approve_change_event_parser(sub_parser: argparse._SubParsersAction):
    __add_approve_parser(sub_parser, "approve-chevent",
                         "approve change event", "change event id", approve_change_event)

def submit_change_event(args: argparse.Namespace):
    logger.debug(f"{submit_change_event.__name__} is called")
    context = __init_context(args)
    ret = context.vsms_admin__change_event__submit(args.id)
    __output_response(ret)
    return

def add_submit_change_event_parser(sub_parser: argparse._SubParsersAction):
    __add_submit_parser(sub_parser, "submit-chevent",
                        "submit change event", "change event id", submit_change_event)

def update_chevent_package(args: argparse.Namespace):
    logger.debug(f"{update_chevent_package.__name__} is called")

    diffjson = None
    if args.diffjson:
        with open(args.diffjson, mode="r") as f:
            diffjson = f.read()

    context = __init_context(args)
    ret = context.vsms_admin__change_event_package__update(
        args.id, args.status, args.name, diffjson, args.diffname)
    __output_response(ret)

    return

def add_update_chevent_package_parser(sub_parser: argparse._SubParsersAction):
    parser = sub_parser.add_parser("update-cheventpkg",
                                   help="update change event package",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in (
        [("id",),
         dict(help=f"server generated change event package id",
              type=str)],
        [("status",),
         dict(help=f"change event package status",
              choices=["DRAFT", "SAVED", "PENDING"],
              type=str)],
        [("--name",),
         dict(help=f"change event package name ( 255 limit )",
              default=None,
              type=str)],
        [("--diffjson",),
         dict(help=f"diffjson file path",
              default=None,
              type=str)],
        [("--diffname",),
         dict(help=f"diff json file name ( 255 limit )",
              default=None,
              type=str)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=update_chevent_package)

def approve_chevent_package(args: argparse.Namespace):
    logger.debug(f"{approve_chevent_package.__name__} is called")
    context = __init_context(args)
    ret = context.vsms_admin__change_event_package__approve(args.id)
    __output_response(ret)
    return

def add_approve_chevent_package_parser(sub_parser: argparse._SubParsersAction):
    __add_approve_parser(sub_parser, "approve-cheventpkg", "approve change event package",
                         "change event pacakge id", approve_chevent_package)

def delete_change_event_package(args: argparse.Namespace):
    logger.debug(f"{delete_change_event_package.__name__} is called")
    context = __init_context(args)

    ret = context.vsms_admin__change_event_package__delete(args.id)
    __output_response(ret)
    return

def create_campaign(args: argparse.Namespace):
    logger.debug(f"{create_campaign.__name__} is called")
    context = __init_context(args)
    ret = context.vsms_admin__campaign__create(
        args.name, args.description if args.description is not None else args.name, args.type)
    __output_response(ret)
    return

def add_create_campaign_parser(sub_parser: argparse._SubParsersAction):
    parser = sub_parser.add_parser("create-campaign",
                                   help="create campaign",
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in (
        [("name",),
         dict(help=f"campaign name ( 128 limit )",
              type=str)],
        [("--description",),
         dict(help=f"description ( 1024 limit )",
              default=None,
              type=str)],
        [("--type",),
         dict(help=f"campaign type",
              default="OTA",
              choices=["INFORMATIONAL", "OTA"],
              type=str)],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=create_campaign)


def submit_campaign(args: argparse.Namespace):
    logger.debug(f"{submit_campaign.__name__} is called")
    context = __init_context(args)

    release_date = None
    if args.release_date is not None:
        release_date = args.release_date
    else:
        # handle release date as JST (TODO: adjust by using timezone not only JST)
        delta = datetime.timedelta(seconds=30)
        release_date = datetime.datetime.now(
            datetime.timezone(datetime.timedelta(hours=9)))+delta
        release_date = release_date.isoformat(timespec='seconds')

    ret = context.vsms_admin__campaign__submit_for_approval(
        args.id, release_date)

    __output_response(ret)
    return

def __iso8601_datetime_type(input: str):
    try:
        datetime.date.fromisoformat(input)
    except ValueError:
        raise argparse.ArgumentTypeError(f"input string {input} is not a valid ISO8601 format"
                                         )

def __add_campaign_status_update_parser(sub_parser: argparse._SubParsersAction, cmd_name: str, help_cmd_msg: str, default_func):
    parser = sub_parser.add_parser(cmd_name,
                                   help=help_cmd_msg,
                                   formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    for names, opts in (
        [("id",),
         dict(help=f"campaign id",
              type=str)],
        [("--release-date",),
         dict(help=f"release date ( ISO8601 format), if not set release date is set to in 30 seconds",
              default=None,
              type=__iso8601_datetime_type)],
            #  TODO: add support later
        # [("--expiration-date",),
        #  dict(help=f"expiration date ( iso8601 format ) ",
        #       default=None,
        #       type=__iso8601_datetime_type)],
        # [("--check-mismatch",),
        #  dict(help=f"check campaign mismatch",
        #       action="store_true")],
    ):
        parser.add_argument(*names, **opts)
    parser.set_defaults(func=default_func)

def add_submit_campaign_parser(sub_parser: argparse._SubParsersAction):
    __add_campaign_status_update_parser(
        sub_parser, "submit-campaign", "submit campaign", submit_campaign)

def approve_campaign(args: argparse.Namespace):
    logger.debug(f"{submit_campaign.__name__} is called")
    context = __init_context(args)

    release_date = None
    if args.release_date is not None:
        release_date = args.release_date
    else:
        # handle release date as JST (TODO: adjust by using timezone not only JST)
        delta = datetime.timedelta(seconds=30)
        release_date = datetime.datetime.now(
            datetime.timezone(datetime.timedelta(hours=9)))+delta
        release_date = release_date.isoformat(timespec='seconds')

    ret = context.vsms_admin__campaign__approval(
        args.id, release_date)

    __output_response(ret)
    return

def add_approve_campaign_parser(sub_parser: argparse._SubParsersAction):
    __add_campaign_status_update_parser(
        sub_parser, "approve-campaign", "approve campaign", approve_campaign)

def cancel_campaign(args: argparse.Namespace):
    logger.debug(f"{cancel_campaign.__name__} is called")
    context = __init_context(args)

    ret = context.vsms_admin__campaign__cancel(args.id)
    __output_response(ret)
    return

def add_cancel_campaign_parser(sub_parser: argparse._SubParsersAction):
    __add_cancel_parser(
        sub_parser, "cancel-campaign", "cancel campaign", "campaign id", cancel_campaign)
