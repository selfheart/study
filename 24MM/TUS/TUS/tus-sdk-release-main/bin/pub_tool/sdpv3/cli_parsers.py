import argparse
import json
import logging
import os
import time
from typing import Dict
from .output_writer import OutputWriter

from common.main import add_id_parser, construct_out_parser_options, _PrintJsonAction, rxswins_parser_options
from common.constants import SDP_PASSWD, SDP_USERNAME, SUBSYSTEM_JSON
from .constants import (
    FILETYPE_LIST, CHANGE_EVENT_MODIFICATION_JSON, CHANGE_EVENT_MODIFICATION,
    COMPONENTS_JSON, VEHICLE_CONF_PART_NUM, VEHICLE_CONF_TARGET_ID, VIN,
    FACTORY_FEED_JSON, SUBSYSDIFF_GENERATION_JSON, SUBSYSDIFF_SYSTEMS,
    MULTI_UPLOAD_DEFAULT_PART_SIZE, MULTI_UPLOAD_MAX_PART_SIZE, MULTI_UPLOAD_MIN_PART_SIZE,
    VSPA_FILETYPE, SEARCH_VEHICLE_KEYS)
from .sdpv3ctx import SDPV3CTX

logger = logging.getLogger(__name__)


def construct_json_parser_options(json_body, json_help_msg: str = "json file path or stdin"):
    return ([("--print-json",),
             dict(help=f"print json format to stdout",
                  default=json_body,
                  action=_PrintJsonAction)],
            [("--json",),
             dict(help=json_help_msg,
                  default=None,
                  type=argparse.FileType("r"),)],)


class CLIParsersV3():
    PREV_RXSWIN = "--prev-rxswin"
    NEW_RXSWIN = "--new-rxswin"
    RXSWIN_SYSTEM_NAME = "--same-rxswin-system-name"
    UPDATED = "--updated"
    output_writer = OutputWriter()

    def __init_context(self, args):
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
            raise ValueError(
                f"Please define {SDP_PASSWD} environment variable.")
        return SDPV3CTX(args.endpoint, username, passwd)

    def __output_response(self, ret: Dict):
        print(json.dumps(ret, indent=2))

    def __add_get_parser(self, sub_parser: argparse._SubParsersAction, cmd_name: str, help_cmd_msg: str, help_id_field_msg: str, default_func):
        parser = sub_parser.add_parser(cmd_name, help=help_cmd_msg)
        for names, opts in (
            [("id",),
             dict(help=help_id_field_msg,
                  type=str)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=default_func)

    def __add_delete_parser(self, sub_parser: argparse._SubParsersAction, cmd_name: str, help_cmd_msg: str, help_id_field_msg: str, default_func):
        add_id_parser(sub_parser, cmd_name, help_cmd_msg,
                      help_id_field_msg, default_func)

    def delete_vehicle_models(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_vehicle_models.__name__} is called")

        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_type__delete(args.modelcode, args.makecode)
        self.__output_response(ret)
        return

    def search_vtype(self, args: argparse.Namespace):
        logger.debug(f"{self.search_vtype.__name__} is called")
        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_type__get(
            model=args.model_code, campaign=args.campaign_id)
        limit = min(len(ret), args.limit)
        if args.print_json:
            self.__output_response(ret[:limit])
            return
        self.output_writer.output_table(ret[:limit])
        return

    def add_search_vtype_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("search-vtype",
                                       help="search vehicle type",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("--model-code",),
             dict(help=f"model code",
                  default=None,
                  type=str)],
            [("--campaign-id",),
             dict(help=f"campaign_id",
                  default=None,
                  type=int)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
            [("--limit",),
             dict(help=f"maximum number to output.",
                  default=100,
                  type=int)],
        ):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.search_vtype)

    def get_vtype(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vtype.__name__} is called")
        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_type__get(
            model=args.modelcode)
        for data in ret:
            if data["modelCode"] == args.modelcode:  # output only the exactly matched results
                if args.print_json:
                    self.__output_response(data)
                else:
                    self.output_writer.write({"id": data.pop("id")})
                    self.output_writer.write(data, level=1)
        return

    def add_get_vtype_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("get-vtype",
                                       help="get vehicle type",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("modelcode",),
             dict(help=f"model code",
                  type=str)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
        ):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.get_vtype)

    def add_delete_vtype_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("delete-vtype",
                                       help="delete vehicle type",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("makecode",),
             dict(help=f"make code",
                  type=str)],
            [("modelcode",),
             dict(help=f"model code",
                  default=None,
                  type=str)],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.delete_vehicle_models)

    def create_vtype_region(self, args: argparse.Namespace):
        logger.debug(f"{self.create_vtype_region.__name__} is called")

        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_type__region__create(args.name)
        self.__output_response(ret)
        return

    def add_create_vtype_region_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("create-vtype-region",
                                       help="create vehicle type region",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("name",),
             dict(help=f"region name",
                  type=str)],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.create_vtype_region)

    def search_vtype_region(self, args: argparse.Namespace):
        logger.debug(f"{self.search_vtype_region.__name__} is called")

        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_type__region__search(args.name)
        limit = min(len(ret), args.limit)
        if args.print_json:
            self.__output_response(ret["results"][:limit])
            return
        self.output_writer.output_table(ret["results"][:limit])
        return

    def add_search_vtype_region_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("search-vtype-region",
                                       help="search vehicle type region",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("--name",),
             dict(help=f"region name",
                  default=None,
                  type=str)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
            [("--limit",),
             dict(help=f"maximum number to output. Cannot be more than 100 due to SDPv3 specification.",
                  default=100,
                  type=int)],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.search_vtype_region)

    def get_vtype_region(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vtype_region.__name__} is called")

        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_type__region__search(args.name)
        for data in ret["results"]:
            if data["name"] == args.name:  # output only the exactly matched results
                if args.print_json:
                    self.__output_response(data)
                else:
                    self.output_writer.write({"id": data.pop("id")})
                    self.output_writer.write(data, level=1)
        return

    def add_get_vtype_region_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("get-vtype-region",
                                       help="get vehicle type region",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("name",),
             dict(help=f"region name",
                  default=None,
                  type=str)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.get_vtype_region)

    def delete_vtype_region(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_vtype_region.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__vehicle_type__region__delete(args.id)
        self.__output_response(ret)

    def add_delete_vtype_region_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_delete_parser(sub_parser, "delete-vtype-region",
                                 "delete vehicle type region", "vehicle type region id", self.delete_vtype_region)

    def create_subsystem(self, args: argparse.Namespace):
        logger.debug(f"{self.create_subsystem.__name__} is called")
        context = self.__init_context(args)

        rxswins = context.parse_rxswins(args.rxswins)
        context.validate_rxswins(rxswins)

        ret = context.vsms_admin__subsystem__create(
            args.name, args.disable_check, rxswins, args.sysId, args.vtype_ids)
        self.__output_response(ret)
        return

    def add_create_subsystem_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("create-subsys",
                                       help="create subsystem",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("name",),
             dict(help=f"subsystem name",
                  default=None,
                  type=str)],
            [("sysId",),
             dict(help=f"subsystem sysId",
                  default=None,
                  type=str)],
            [("--disable-check",),
             dict(help=f"disable system check",
                  action="store_false",
                  )],
            [("--vtype-ids",),
             dict(help=f"vehicle type ids",
                  default=None,
                  nargs="*",
                  type=int
                  )]
        ) + rxswins_parser_options:
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.create_subsystem)

    def get_subsystem(self, args: argparse.Namespace):
        logger.debug(f"{self.get_subsystem.__name__} is called")
        context = self.__init_context(args)
        ret = context.vsms_admin__subsystem__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_subsystem_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-subsys", "get subsystem",
                              "subsystem id", self.get_subsystem)

    def delete_subsystem(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_subsystem.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__subsystem_delete(args.id)
        self.__output_response(ret)
        return

    def add_delete_subsystem_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_delete_parser(sub_parser, "delete-subsys",
                                 "delete subsystem", "subsystem id", self.delete_subsystem)

    def create_hwcomp(self, args: argparse.Namespace):
        logger.debug(f"{self.create_hwcomp.__name__} is called")

        context = self.__init_context(args)
        VEHICLE_CONF_PART_NUM.validate(args.part_num)
        VEHICLE_CONF_TARGET_ID.validate(args.targetid)

        ret = context.vsms_admin__hwcomponent__create(
            args.targetid, args.name, args.repro_platform, args.part_num,
            args.physical_addr, args.rom_part)
        self.__output_response(ret)
        return

    def add_create_hwcomp_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("create-hwcomp",
                                       help="create hardware component",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("targetid",),
             dict(help=f"target id ( 24 limit string ), it shall be able to be converted into octal number.",
                  type=str)],
            [("name",),
             dict(help=f"ECU name ( 50 limit string )",
                  type=str)],
            [("--repro-platform",),
             dict(help=f"repro platform",
                  default="ap",
                  choices=["cp", "ap"],
                  type=str)],
            [("--part-num",),
             dict(help=f"part number",
                  default=None,
                  type=str)],
            [("--physical-addr",),
             dict(help=f"physical address ( 24 limit length )",
                  default=None,
                  type=str)],
            [("--rom-part",),
             dict(help=f"bank",
                  default="DUAL_BANK",
                  choices=["SINGLE_BANK", "DUAL_BANK"],
                  type=str)],
        ):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.create_hwcomp)

    def search_hwcomp(self, args: argparse.Namespace):
        logger.debug(f"{self.search_hwcomp.__name__} is called")

        context = self.__init_context(args)
        if args.ecu_name is not None:
            ret = context.vsms_admin__hwcomponent__get(
                args.subsystem_ids, args.ecu_name)
        elif args.part_num is not None:
            ret = context.vsms_admin__hwcomponent__get(
                args.subsystem_ids, args.part_num)
        elif args.target_id is not None:
            ret = context.vsms_admin__hwcomponent__get(
                args.subsystem_ids, args.target_id)
        else:
            ret = context.vsms_admin__hwcomponent__get(
                args.subsystem_ids, None)

        limit = min(len(ret["results"]), args.limit)

        result = []
        for i in range(limit):
            data = ret["results"][i]
            component = {}
            component["id"] = data["id"]
            for key in ["hardware", "target"]:
                component.update(data[key])
            result.append(component)

        if args.print_json:
            self.__output_response(result)
            return
        self.output_writer.output_table(result)
        return

    def add_search_hwcomp_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("search-hwcomp",
                                       help="search hardware component",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("--subsystem-ids",),
             dict(help=f"subsystem ids",
                  nargs="*",
                  default=None,
                  type=int)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
            [("--limit",),
             dict(help=f"maximum number to output. Cannot be more than 100 due to SDPv3 specification.",
                  default=100,
                  type=int)],
        ):
            parser.add_argument(*names, **opts)

        group = parser.add_mutually_exclusive_group(required=False)
        for name, opts in (
            [("--ecu-name",),
             dict(help=f"ecu name",
                  default=None,
                  type=str)],
            [("--part-num",),
             dict(help=f"part number",
                  default=None,
                  type=str)],
            [("--target-id",),
             dict(help=f"target id",
                  default=None,
                  type=str)],
        ):
            group.add_argument(*name, **opts)

        parser.set_defaults(func=self.search_hwcomp)

    def get_hwcomp(self, args: argparse.Namespace):
        logger.debug(f"{self.get_hwcomp.__name__} is called")

        context = self.__init_context(args)
        param = args.ecu_name or args.part_num or args.target_id
        ret = context.vsms_admin__hwcomponent__get(None, param)

        for data in ret["results"]:
            target = None
            if args.ecu_name is not None:
                target = data["hardware"]["ecuName"]
            elif args.part_num is not None:
                target = data["hardware"]["partNumber"]
            elif args.target_id is not None:
                target = data["target"]["targetId"]
            if target == param:  # output only the exactly matched results
                if args.print_json:
                    self.__output_response(data)
                else:
                    self.output_writer.write({"id": data.pop("id")})
                    self.output_writer.write(data, level=1)

        return

    def add_get_hwcomp_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("get-hwcomp",
                                       help="get hardware component",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
        ):
            parser.add_argument(*names, **opts)

        group = parser.add_mutually_exclusive_group(required=True)
        for name, opts in (
            [("--ecu-name",),
             dict(help=f"ecu name",
                  default=None,
                  type=str)],
            [("--part-num",),
             dict(help=f"part number",
                  default=None,
                  type=str)],
            [("--target-id",),
             dict(help=f"target id",
                  default=None,
                  type=str)],
        ):
            group.add_argument(*name, **opts)

        parser.set_defaults(func=self.get_hwcomp)

    def create_subsystemconf(self, args: argparse.Namespace):
        logger.debug(f"{self.create_subsystemconf.__name__} is called")

        components = None
        if args.json is not None:
            with args.json as f:
                components = json.load(f)
        components = components["components"] if components is not None else None
        context = self.__init_context(args)
        rxswins = context.parse_rxswins(args.rxswins)
        context.validate_rxswins(rxswins)
        context.validate_components(components)
        ret = context.vsms_admin__subsystem_configuration__create(
            args.id, args.compare_sysid, args.sysid, rxswins, components)
        self.__output_response(ret)
        return

    def add_create_subsystemconf_parser(self, sub_parser: argparse._SubParsersAction):
        # if add ArgumentDefaultsHelpFormatter, help message --print-json prints all json value
        parser = sub_parser.add_parser("create-subsysconf",
                                       help="create subsystem configuration")
        for names, opts in (
            [("id",),
             dict(help=f"subsystem id",
                  default=None,
                  type=str)],
            [("sysid",),
             dict(help=f"sys id",
                  type=str)],
            [("--compare-sysid",),
             dict(help=f"compare sys id",
                  default=None,
                  type=str)],
            [("--vtype-ids",),
             dict(help=f"vehicle type ids",
                  default=None,
                  nargs="*",
                  type=int
                  )]
        ) + rxswins_parser_options + construct_json_parser_options(COMPONENTS_JSON):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.create_subsystemconf)

    def update_subsystemconf(self, args: argparse.Namespace):
        logger.debug(f"{self.update_subsystemconf.__name__} is called")

        components = None
        if args.json is not None:
            with args.json as f:
                components = json.load(f)
        components = components["components"] if components is not None else None
        context = self.__init_context(args)
        rxswins = context.parse_rxswins(args.rxswins)
        context.validate_rxswins(rxswins)
        context.validate_components(components)

        ret = context.vsms_admin__subsystem_configuration__edit(
            args.id, args.sysid, rxswins, components)
        self.__output_response(ret)
        return

    def add_update_subsystemconf_parser(self, sub_parser: argparse._SubParsersAction):
        # if add ArgumentDefaultsHelpFormatter, help message --print-json prints all json value
        parser = sub_parser.add_parser("update-subsysconf",
                                       help="update subsystem configuration")
        for names, opts in (
            [("id",),
             dict(help=f"subsystem configuration id",
                  default=None,
                  type=int)],
            [("--sysid",),
             dict(help=f"sys id",
                  default=None,
                  type=str)],
            [("--vtype-ids",),
             dict(help=f"vehicle type ids",
                  default=None,
                  nargs="*",
                  type=int
                  )]
        ) + rxswins_parser_options + construct_json_parser_options(COMPONENTS_JSON):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.update_subsystemconf)

    def create_vdesign(self, args: argparse.Namespace):
        logger.debug(f"{self.create_vdesign.__name__} is called")

        subsystem = None
        if args.json is not None:
            with args.json as f:
                subsystem = json.load(f)

        if args.prev_rxswin is not None or args.new_rxswin is not None or args.same_rxswin_system_name is not None or args.updated is not None:
            if args.new_rxswin is None or args.same_rxswin_system_name is None:
                raise ValueError(
                    f"if rxswin related parameters are passed, {CLIParsersV3.NEW_RXSWIN} and {CLIParsersV3.RXSWIN_SYSTEM_NAME} are mandatory.")

        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_design__create(
            args.ids, args.status, subsystem.get("subsystems") if subsystem is not None else None, args.prev_vdesign_id, args.prev_rxswin, args.new_rxswin, args.same_rxswin_system_name, args.updated)
        self.__output_response(ret)
        return

    def add_create_vdesign_parser(self, sub_parser: argparse._SubParsersAction):
        # if add ArgumentDefaultsHelpFormatter, help message --print-json prints all json value
        parser = sub_parser.add_parser("create-vdesign",
                                       help="create vehicle design")
        for names, opts in (
            [("ids",),
             dict(help=f"vehicle type ids",
                  nargs="+",
                  type=int)],
            [("--status",),
             dict(help=f"vehicle design status. default='PENDING'",
                  default="PENDING",
                  choices=["DRAFT", "PENDING"],
                  type=str)],
            [("--prev-vdesign-id",),
             dict(help=f"previous vehicle design id",
                  type=int)],
        ) + construct_json_parser_options(SUBSYSTEM_JSON, "json filepath to input subsystem"):
            parser.add_argument(*names, **opts)

        parser_group = parser.add_argument_group(
            "rxswin update info", f"""
            update info related rxswin.
            If one of {CLIParsersV3.PREV_RXSWIN}, {CLIParsersV3.NEW_RXSWIN}, {CLIParsersV3.RXSWIN_SYSTEM_NAME} and {CLIParsersV3.UPDATED} are passed,
            {CLIParsersV3.NEW_RXSWIN} and {CLIParsersV3.RXSWIN_SYSTEM_NAME} are mandatory.
            """)
        for names, opts in (
            [(CLIParsersV3.PREV_RXSWIN,),
             dict(help=f"previous rxswin",
                  default=None,
                  type=str)],
            [(CLIParsersV3.NEW_RXSWIN,),
             dict(help=f"new rxswin",
                  default=None,
                  type=str)],
            [(CLIParsersV3.RXSWIN_SYSTEM_NAME,),
             dict(help=f"system information which caused the update in other system",
                  default=None,
                  type=str)],
            [(CLIParsersV3.UPDATED,),
             dict(help=f"user's input for the update",
                  default=None,
                  type=bool)],
        ):
            parser_group.add_argument(*names, **opts)
        parser.set_defaults(func=self.create_vdesign)

    def get_vdesign(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vdesign.__name__} is called")
        context = self.__init_context(args)
        ret = context.vsms_admin__vehicle_design__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_vdesign_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-vdesign", "get vehicle design",
                              "vehicle design id", self.get_vdesign)

    def delete_vdesign(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_vdesign.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__vdesign__delete(args.id)
        self.__output_response(ret)
        return

    def add_delete_vdesign_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_delete_parser(sub_parser, "delete-vdesign",
                                 "delete vehicle design", "vehicle design id", self.delete_vdesign)

    def create_vehicle(self, args: argparse.Namespace):
        logger.debug(f"{self.create_vehicle.__name__} is called")
        if args.json is None:
            raise ValueError("Please pass --json to input")

        if args.public_key is None:
            raise ValueError("Please pass public key file")

        vehicles = None
        with args.json as f:
            vehicles = json.load(f)

        with open(args.public_key) as f:
            public_key = f.read()

        context = self.__init_context(args)
        context.validate_vehicle_json(vehicles, public_key)
        ret = context.factory_feed__post(vehicles)
        self.__output_response(ret)
        return

    def add_create_vehicle_parser(self, sub_parser: argparse._SubParsersAction):
        # if add ArgumentDefaultsHelpFormatter, help message --print-json prints all json value
        parser = sub_parser.add_parser("create-vehicle",
                                       help="create vehicle and new VINs")
        for names, opts in (
            [("--public-key",),
             dict(help=f"public key file path for primary ECU",
                  type=str)],
        ) + construct_json_parser_options(FACTORY_FEED_JSON):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.create_vehicle)

    def search_vehicle(self, args: argparse.Namespace):
        logger.debug(f"{self.search_vehicle.__name__} is called")
        context = self.__init_context(args)
        ret = context.factory_feed__get(args.vin)
        limit = min(len(ret), args.limit)
        ret = ret["results"][:limit]
        # limit key to improve UI/UX
        result = []
        for data in ret:
            component = {}
            for key in SEARCH_VEHICLE_KEYS:
                try:
                    component[key] = data[key]
                except KeyError:
                    component[key] = ""
            result.append(component)
        if args.print_json:
            self.__output_response(result)
            return
        self.output_writer.output_table(result)
        return

    def add_search_vehicle_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("search-vehicle",
                                       help="search vehicle",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("--vin",),
             dict(help=f"case insensitive complete match or substring match",
                  type=str)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
            [("--limit",),
             dict(help=f"maximum number to output.",
                  default=100,
                  type=int)],
        ):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.search_vehicle)

    def get_vehicle(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vehicle.__name__} is called")
        context = self.__init_context(args)
        ret = context.factory_feed__get(args.vin)

        for data in ret["results"]:
            if data["vin"].lower() == args.vin.lower():  # output only the exactly matched results
                if args.print_json:
                    self.__output_response(data)
                else:
                    self.output_writer.write(data)
        return

    def add_get_vehicle_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("get-vehicle",
                                       help="get vehicle",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("vin",),
             dict(help=f"case insensitive complete match or substring match",
                  type=str)],
            [("--print-json",),
             dict(help=f"print json format to stdout",
                  action='store_true')],
        ):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.get_vehicle)

    def delete_vehicle(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_vehicle.__name__} is called")

        context = self.__init_context(args)

        ret = context.factory_feed__delete(args.vin, args.event_id, args.is_confirm)
        self.__output_response(ret)
        return

    def add_delete_vehicle_parser(self, sub_parser: argparse._SubParsersAction):
        # if add ArgumentDefaultsHelpFormatter, help message --print-json prints all json value
        parser = sub_parser.add_parser("delete-vehicle",
                                       help="delete vehicle",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("vin",),
             dict(help=f"VIN",
                  type=str)],
            [("event_id",),
             dict(help=f"event id",
                  metavar="event-id",
                  type=str)],
            [("--is-confirm",),
             dict(help=f"require confirmation by Admin before deleting",
                  default=False,
                  type=bool)],
        ):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.delete_vehicle)

    __change_event_common_parser_options = ([("--name",),
                                             dict(help=f"change event name, if not specified chevent-id is used.",
                                                  default=None,
                                                  type=str)],
                                            [("--description",),
                                             dict(help=f"change event description if not specified chevent-id is used",
                                                  default=None,
                                                  type=str)],
                                            [("--pkg-id",),
                                             dict(help=f"package id",
                                                  default=None,
                                                  type=int)],
                                            [("--depend-sysids",),
                                             dict(help=f"sysid list of dependent systems associated to change event",
                                                  default=None,
                                                  nargs="*",
                                                  type=str)],

                                            [("--authority-report-id",),
                                             dict(help=f"authority report id",
                                                  default=None,
                                                  type=str)],
                                            [("--reason-class",),
                                             dict(help=f"reason class (default: 9)",
                                                  default=9,
                                                  type=int)],
                                            [("--reason-content",),
                                             dict(help=f"reason content",
                                                  default=None,
                                                  type=str)])

    # only creation API
    __change_event_create_parser_options = (
        [("--type",),
         dict(help=f"change event type",
              default="SOFTWARE_UPDATE",
              choices=["INFORMATIONAL",
                       "SOFTWARE_UPDATE"],
              type=str)],
        [("--status",),
         dict(help=f"change event status",
              default="DRAFT",
              choices=["DRAFT",
                       "PENDING_APPROVAL"],
              type=str)])

    def create_change_event(self, args: argparse.Namespace):
        logger.debug(f"{self.create_change_event.__name__} is called")
        context = self.__init_context(args)
        mods_json = None
        if args.json:
            with args.json as f:
                mods_json = json.load(f)
                # raise error if invalid
                context.validate_modifications_json(mods_json)

        ret = context.vsms_admin__change_event__create(
            args.chevent_id, args.name if args.name is not None else args.chevent_id, args.description if args.description is not None else args.chevent_id, args.type,
            args.pkg_id, args.authority_report_id, args.reason_class, args.reason_content, args.status, args.depend_sysids,
            mods_json[CHANGE_EVENT_MODIFICATION.key] if mods_json is not None else None,
        )
        self.__output_response(ret)
        return

    def add_create_change_event_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("create-chevent",
                                       help="create change event",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("chevent_id",),
             dict(help="user defined change event id",
                  metavar="chevent-id",
                  type=str)],
        ) + self.__change_event_common_parser_options + self.__change_event_create_parser_options + construct_json_parser_options(CHANGE_EVENT_MODIFICATION_JSON):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.create_change_event)

    def update_change_event(self, args: argparse.Namespace):
        logger.debug(f"{self.update_change_event.__name__} is called")
        context = self.__init_context(args)
        mods_json = None
        if args.json:
            with args.json as f:
                mods_json = json.load(f)
                # raise error if invalid
                context.validate_modifications_json(mods_json)

        ret = context.vsms_admin__change_event__update(
            args.id, args.chevent_id, args.name if args.name is not None else args.chevent_id, args.description if args.description is not None else args.chevent_id,
            args.pkg_id, args.authority_report_id, args.reason_class, args.depend_sysids,
            mods_json[CHANGE_EVENT_MODIFICATION.key] if mods_json is not None else None,
        )
        self.__output_response(ret)
        return

    def add_update_change_event_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("update-chevent",
                                       help="update change event",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("id",),
             dict(help="server generated change event id",
                  type=str)],
            [("chevent_id",),
             dict(help="user defined change event id",
                  metavar="chevent-id",
                  type=str)],
        ) + self.__change_event_common_parser_options + construct_json_parser_options(CHANGE_EVENT_MODIFICATION_JSON):
            parser.add_argument(*names, **opts)

        parser.set_defaults(func=self.update_change_event)

    def get_change_event(self, args: argparse.Namespace):
        logger.debug(f"{self.get_change_event.__name__} is called")
        context = self.__init_context(args)
        ret = context.vsms_admin__change_event__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_change_event_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-chevent", "get change event",
                              "change event id", self.get_change_event)

    def get_chevent_package(self, args: argparse.Namespace):
        logger.debug(f"{self.get_chevent_package.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__change_event_package__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_chevent_package_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-cheventpkg", "get change event package",
                              "change event package id", self.get_chevent_package)

    def delete_change_event(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_change_event.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__change_event__delete(args.id)
        self.__output_response(ret)
        return

    def add_delete_change_event_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_delete_parser(sub_parser, "delete-chevent",
                                 "delete change event", "change event id", self.delete_change_event)

    def create_subsysdiff(self, args: argparse.Namespace):
        logger.debug(f"{self.create_subsysdiff.__name__} is called")
        context = self.__init_context(args)
        systems_json = None
        if args.json:
            with args.json as f:
                systems_json = json.load(f)
                systems = systems_json.get(SUBSYSDIFF_SYSTEMS.key)
                SUBSYSDIFF_SYSTEMS.validate(systems)
                context.validate_system_info(systems)

        ret = context.vsms_admin__subsystem__diff(systems_json.get(SUBSYSDIFF_SYSTEMS.key), args.depend_sysids)
        if args.out is None:
            self.__output_response(ret)
        else:
            with open(args.out, mode="w") as f:
                # set remove additional spaces for creating change event package
                f.write(json.dumps(ret, separators=(',', ':')))
            logger.info(f"write return to file {args.out}")
        return

    def add_create_subsysdiff_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("create-subsysdiff",
                                       help="create subsystem diff",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("--depend-sysids",),
             dict(help=f"sysid list of dependent systems associated to change event",
                  default=None,
                  nargs="*",
                  type=str)],
        ) + construct_out_parser_options() + construct_json_parser_options(SUBSYSDIFF_GENERATION_JSON):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.create_subsysdiff)

    def create_chevent_package(self, args: argparse.Namespace):
        logger.debug(f"{self.create_chevent_package.__name__} is called")
        context = self.__init_context(args)

        with open(args.diffjson, mode="r") as f:
            ret = context.vsms_admin__change_event_package__create(
                args.name, f.read(), args.diffname)
            self.__output_response(ret)

    def add_create_chevent_package_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("create-cheventpkg",
                                       help="create change event package.",
                                       description=f"create change event package."
                                       f" Before creating change event package, change event shall be created."
                                       f" If there are no change events which has same sysId difference, this fails to create change event package. "
                                       f" For uploading TUP itself, please call 'tus pdp sdpv3 ul-pkg' after creating change event package.",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("name",),
             dict(help=f"change event package name ( 255 limit )",
                  type=str)],
            [("diffjson",),
             dict(help=f"input diff json file path. diff json could be get by create-subsysdiff subcommand",
                  type=str)],
            [("--diffname",),
             dict(help=f"diff json file name ( 255 limit )",
                  default="diff.json",
                  type=str)],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.create_chevent_package)

    def delete_change_event_package(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_change_event_package.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__change_event_package__delete(args.id)
        self.__output_response(ret)
        return

    def add_delete_change_event_package_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_delete_parser(sub_parser, "delete-cheventpkg",
                                 "delete change event package", "change event package id", self.delete_change_event_package)

    def get_campaign(self, args: argparse.Namespace):
        logger.debug(f"{self.get_campaign.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__campaign__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_campaign_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-campaign", "get campaign",
                              "campaign id", self.get_campaign)

    def delete_campaign(self, args: argparse.Namespace):
        logger.debug(f"{self.delete_campaign.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__campaign__delete(args.id)
        self.__output_response(ret)

    def update_campaign(self, args: argparse.Namespace):
        logger.debug(f"{self.update_campaign.__name__} is called")
        context = self.__init_context(args)

        if args.chevents:
            if len(args.chevents) % 2 != 0:
                raise ValueError("chevents shall be pair.")
            context.vsms_admin__campaign__modify_change_event(
                args.id, args.chevents)
        elif args.vins:
            for vin in args.vins:
                VIN.validate(vin)
            context.vsms_admin__campaign__add_vins(args.id, args.vins)
        elif args.vtypes:
            context.vsms_admin__campaign__modify_vehicle_types(
                args.id, args.vtypes)
        elif args.message_file:
            if not os.path.exists(args.message_file):
                raise ValueError(f"Could not find {args.message_file}")
            with open(args.message_file, "rb") as f:
                ret = context.vsms_admin__campaign__add_localized_messages(campaign_id=args.id, file_name=os.path.basename(args.message_file), file_obj=f.read())
                self.__output_response(ret)
        else:
            raise ValueError("do not reach here")
        return

    def add_update_campaign_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("update-campaign",
                                       help="update campaign",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        # TODO: add more types later
        for names, opts in (
            [("id",),
             dict(help=f"campaign id",
             type=str)],
        ):
            parser.add_argument(*names, **opts)

        group = parser.add_mutually_exclusive_group(required=True)
        for name, opts in (
            [("--chevents",),
             dict(help=f"change event id and their priority pairs, e.g.) --chevents chevent_id_1 0 chevent_id_2 0 ...",
             nargs="+",
             type=int)],
            [("--vins",),
             dict(help=f"vin lists",
             nargs="+",
             type=str)],
            [("--vtypes",),
             dict(help=f"vehicle type ids",
             nargs="+",
             type=str)],
            [("--message-file",),
             dict(help="csv format file containing localized messages.\
                        each line should contain the language ISO code with locale ID, key and the message.\
                        all keys should be repeated for all provided languages in the file.",
                  metavar="PATH",
                  type=str,
                  default=None)]
        ):
            group.add_argument(*name, **opts)

        parser.set_defaults(func=self.update_campaign)

    def add_delete_campaign_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_delete_parser(sub_parser, "delete-campaign",
                                 "delete campaign, only DRAFT, PENDING_APPROVAL, APPROVED, CHANGE_REQUESTED status can be deleted.", "campaign id", self.delete_campaign)

    def upload_pkg_one_shot(self, args: argparse.Namespace):
        logger.debug(f"{self.upload_pkg_one_shot.__name__} is called")
        context = self.__init_context(args)

        # get_upload_info
        ret = context.vsms_admin__change_event_package__get_upload_info(
            args.id, args.file_path, os.path.basename(args.file_path), args.file_type)
        upload_id = ret.get("id")

        # post package file
        upload_url = ret.get("uploadURL")
        logger.info(f"post packagefile to {upload_url}")
        context.vsms_admin__change_event_package__upload_package(
            upload_url, args.file_path, args.file_type)

        # notify backend that uploading is completed.
        logger.info(f"notify uploading is finished, upload_id:{upload_id}")
        upload_location = context.vsms_admin__package__process(upload_id)
        self.__output_response({"upload_location": upload_location})

        return upload_location

    def upload_pkg_multi_part(self, args: argparse.Namespace):
        logger.debug(f"{self.upload_pkg_multi_part.__name__} is called")
        context = self.__init_context(args)

        context.validate_upload_part_size(args.part_size)
        base64_md5_list = context.get_base64_md5_list(args.file_path, args.file_type, args.part_size)
        context.validate_upload_part_num(len(base64_md5_list))

        # initiate multi part upload
        initiate = context.vsms_admin__initiate_multi_part_upload(
            args.id, os.path.basename(args.file_path), args.file_type, base64_md5_list, args.file_hash_value, args.target_id)
        upload_id = initiate.get("uploadId")
        upload_context = initiate.get("uploadContext")
        part_ids = []
        upload_file = context.open_upload_file(args.file_path, args.file_type)
        for i in range(len(base64_md5_list)):
            content = upload_file.read(args.part_size)
            upload_url = initiate.get("uploadParts")[i].get("uploadURL")
            logger.info(f"post packagefile to {upload_url}")
            ret = context.vsms_admin__upload_part(upload_url, content, base64_md5_list[i])
            part_ids.append(ret.get("ETag"))
        upload_file.close()
        context.vsms_admin__complete_multi_part_upload(upload_context, part_ids)

        # notify backend that uploading is completed.
        logger.info(f"notify uploading is finished, upload_id:{upload_id}")
        upload_location = context.vsms_admin__package__process(upload_id)
        self.__output_response({"upload_location": upload_location})

        return upload_location

    def get_package_processing_status(self, args: argparse.Namespace):
        logger.debug(f"{self.get_package_processing_status.__name__} is called")
        context = self.__init_context(args)

        ret = context.vsms_admin__package__get_status(args.upload_location)
        self.__output_response(ret)

        return

    def approve_package(self, args: argparse.Namespace):
        logger.debug(f"{self.approve_package.__name__} is called")
        context = self.__init_context(args)
        logger.info("updating change event package status to PENDING and approve it.")
        context.vsms_admin__change_event_package__update(args.id, "PENDING")
        context.vsms_admin__change_event_package__approve(args.id)
        logger.info("Approved change event package successfully!")

        return

    def poll_package_processing(self, args: argparse.Namespace, upload_location):
        logger.debug(f"{self.poll_package_processing.__name__} is called")
        context = self.__init_context(args)

        # polling package process.
        logger.info(f"package processing location:{upload_location}")
        while True:
            logger.info(f"wait for process uploading software package...")
            res = context.vsms_admin__package__get_status(upload_location)
            processing_status = res.get("status")
            if processing_status:
                logger.info(f"current status: {processing_status}")
                if processing_status == "FAILED":
                    logger.error(
                        f"process uploading software package, but got error message from system error_code:{res.get('errorCode')}, error_message:{res.get('errorMessage')}")
                    exit(1)
                elif processing_status == "PROCESSING" or processing_status == "PENDING":
                    time.sleep(3)
                elif processing_status == "COMPLETED":
                    logger.info(f"finish processing software package!")
                    break
                elif processing_status == "CANCELLED":
                    logger.error(
                        f"change event package id:{args.id} is cancelled.")
                    exit(1)
                else:
                    logger.error(
                        f"unknown error occurred. server response processing status:{processing_status}")
                    exit(1)
            else:
                logger.error(
                    f"process uploading software package, but it fails to upload.")
                exit(1)

        return

    def pkg_upload(self, args: argparse.Namespace):
        logger.debug(f"{self.pkg_upload.__name__} is called")
        upload_location = self.upload_pkg_one_shot(args)
        self.poll_package_processing(args, upload_location)

        return

    def pkg_multi_part_upload(self, args: argparse.Namespace):
        logger.debug(f"{self.pkg_multi_part_upload.__name__} is called")
        upload_location = self.upload_pkg_multi_part(args)
        if not args.async_ops or args.approve:
            logger.info("poll package processing called.")
            self.poll_package_processing(args, upload_location)
        # approve
        if args.approve:
            self.approve_package(args)
        else:
            logger.info(
                "after uploading package, please update this change event package status to PENDING and approve it manually.")

        return

    def add_pkg_upload_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("ul-pkg",
                                       help="upload package files including TUP to pdp.",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("id",),
             dict(help=f"change event package id",
                  type=str)],
            [("file_path",),
             dict(help=f"package file path, if {VSPA_FILETYPE} is passed this file is archived by ZIP format automatically.",
                  metavar="file-path",
                  type=str)],
            [("--file-type",),
             dict(help=f"file type",
                  default=VSPA_FILETYPE,
                  type=str,
                  choices=FILETYPE_LIST)],
            [("--file_hash_value",),
             dict(help=f"Hash Values of the file. This is mandatory for specific file types like REPRO_UPDATE, ROLLBACK_DATA etc.",
                  type=str,
                  default=None)],
            [("--target_id",),
             dict(help=f"target id. This is mandatory for specific file types like REPRO_UPDATE, ROLLBACK_DATA etc.",
                  type=str,
                  default=None)],
            [("--part_size",),
             dict(help=f"Maximum size of a content to upload on a each single upload. Should be within {MULTI_UPLOAD_MIN_PART_SIZE} and {MULTI_UPLOAD_MAX_PART_SIZE} byte",
                  type=str,
                  default=MULTI_UPLOAD_DEFAULT_PART_SIZE)],
            [("--approve",),
             dict(help=f"if this option is passed, after uploading package this automatically approves.",
                  action='store_true')],
            [("--async_ops",),
             dict(help=f"if this option is passed, it will upload packages without checking upload status on pdp. \
                  if --approve is set as well, it will wait until uploaded package status becomes approvable.",
                  action='store_true')],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.pkg_multi_part_upload)

    def add_get_package_processing_status_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("get-pkg-status",
                                       help="check status of uploaded file on pdp.",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("upload_location",),
             dict(help=f"location of uploaded file on pdp.",
                  type=str)],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.get_package_processing_status)

    def add_approve_package_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("approve-pkg",
                                       help="approve uploaded file on pdp.",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("id",),
             dict(help=f"change event package id",
                  type=str)],
        ):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.approve_package)

    def pkg_download(self, args: argparse.Namespace):
        logger.debug(f"{self.pkg_download.__name__} is called")

        # get_download info
        context = self.__init_context(args)
        ret = context.vsms_admin__change_event_package__get_download_info(
            args.id, args.file_type)
        download_url = ret.get("url")
        context.download_file(download_url, args.out, args.file_type)

        return

    def add_pkg_download_parser(self, sub_parser: argparse._SubParsersAction):
        parser = sub_parser.add_parser("dl-pkg",
                                       help="download package files including TUP",
                                       formatter_class=argparse.ArgumentDefaultsHelpFormatter)
        for names, opts in (
            [("id",),
             dict(help=f"change event package id",
                  type=str)],
            [("--file-type",),
             dict(help=f"file type",
                  default=VSPA_FILETYPE,
                  type=str,
                  choices=FILETYPE_LIST)],
        ) + construct_out_parser_options(required=True):
            parser.add_argument(*names, **opts)
        parser.set_defaults(func=self.pkg_download)

    def get_vehicle_sysevent(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vehicle_sysevent.__name__} is called")
        context = self.__init_context(args)

        ret = context.help_desk__systemevents__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_vehicle_sysevent_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-vin-sysevent", "get vehicle system events",
                              "Vehicle Identification Number", self.get_vehicle_sysevent)

    def get_vehicle_config(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vehicle_config.__name__} is called")
        context = self.__init_context(args)

        ret = context.help_desk__current_config__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_vehicle_config_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-vin-currconf", "get vehicle current configuration",
                              "Vehicle Identification Number", self.get_vehicle_config)

    def get_vehicle_config_info(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vehicle_config.__name__} is called")
        context = self.__init_context(args)

        ret = context.help_desk__current_config__get(args.id)
        result = {}
        if ret is not None and ret != {}:
            result["vin"] = args.id
            result["rxswins"] = ret["reportedRxswins"]
            result["configurations"] = []
            for sysconf in ret["systemConfigurations"]:
                for hwcomp in sysconf["hardwareComponents"]:
                    for sw in hwcomp["softwares"]:
                        result["configurations"].append({
                            "targetId": hwcomp["targetId"],
                            "partNumber": hwcomp["partNumber"],
                            "subTargetId": sw["subTargetId"],
                            "productNumber(softwareId)": sw["softwareId"]
                        })
        if args.print_json:
            self.__output_response(result)
            return
        self.output_writer.write(result)
        return

    def add_get_vehicle_config_info_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-vconf-info", "get current vehicle configuration information",
                              "Vehicle Identification Number", self.get_vehicle_config_info)

    def get_vehicle_histupdate(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vehicle_histupdate.__name__} is called")
        context = self.__init_context(args)

        ret = context.help_desk__campaign_updates__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_vehicle_histupdate_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-vin-histupdate", "get vehicle campaign update history",
                              "Vehicle Identification Number", self.get_vehicle_histupdate)

    def get_vehicle_histconfig(self, args: argparse.Namespace):
        logger.debug(f"{self.get_vehicle_histconfig.__name__} is called")
        context = self.__init_context(args)

        ret = context.help_desk__history_config__get(args.id)
        if args.print_json:
            self.__output_response(ret)
            return
        self.output_writer.write(ret)
        return

    def add_get_vehicle_histconfig_parser(self, sub_parser: argparse._SubParsersAction):
        self.__add_get_parser(sub_parser, "get-vin-histconf", "get vehicle config history",
                              "Vehicle Identification Number", self.get_vehicle_histconfig)
