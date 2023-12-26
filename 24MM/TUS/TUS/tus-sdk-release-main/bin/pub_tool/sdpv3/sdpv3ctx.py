import base64
import hashlib
import os
import tempfile
import zipfile

import requests
from common.ctx import CTX
from .constants import (
    CHANGE_EVENT_MODIFICATION, COMPONENTS_JSON_ID, COMPONENTS_JSON_NOTE,
    COMPONENTS_JSON_PRODUCT_NUMBER, COMPONENTS_JSON_REPRO_DELIVERY, COMPONENTS_JSON_SOFT_INFO,
    COMPONENTS_JSON_SUBTARGET_ID, EVENT_TYPE, EVENTID,
    MULTI_UPLOAD_MAX_PART_NUM, MULTI_UPLOAD_MAX_PART_SIZE, MULTI_UPLOAD_MIN_PART_SIZE,
    PHONE_NUMBER, PRIMARY_ELEMENTID,
    REGION, REPRO_DELIVERY_METHOD_CAND, SYSTEM_INFORMATION_RESULT_SYSID,
    SYSTEM_INFORMATION_START_SYSID, SYSTEM_INFORMATION_ELEMENT_CONFIG_SW, SYSTEM_INFORMATION_STARTS,
    SYSTEM_INFORMATION_SUBTARGET_REPROS, SYSTEM_INFORMATION_REPRO_TYPE, SYSTEM_INFORMATION_REPRO_TYPE_LIST,
    VEHICLE_COLOR, VEHICLE_CONF_ELEMENT_ID, VEHICLE_CONF_PART_NUM,
    VEHICLE_CONF_PRODUCT_NUMBER, VEHICLE_CONF_PUBLIC_KEY_ID,
    VEHICLE_CONF_SOFTWAREINFOS, VEHICLE_CONF_SUBTARGET_ID, VEHICLE_CONF_TARGET_ID,
    VEHICLE_CONFS, VEHICLE_INFOS, VEHICLE_MAKE, VEHICLE_MODEL,
    VEHICLE_MODELYEAR, VEHICLE_TRIM, VIN, VSPA_FILETYPE
)
import logging
from typing import List
logger = logging.getLogger(__name__)


class SDPV3CTX(CTX):
    def __raise_invalid_json_parameter(self, keyname: str, length: int = None, mandatory: bool = True):
        if mandatory:
            if length:
                raise ValueError(
                    f"{keyname} shall be defined and less than {length}.")
            else:
                raise ValueError(
                    f"{keyname} shall be defined.")
        else:
            if length:
                raise ValueError(f"{keyname} shall be less than {length}.")
            else:
                # do not reach here
                pass

    def __validate_vehicle_conf(self, v_confs, primary_id, public_key):
        found_primary_element = False
        for conf in v_confs:
            element_id = conf.get(VEHICLE_CONF_ELEMENT_ID.key)
            VEHICLE_CONF_ELEMENT_ID.validate(element_id)
            logger.debug(f"vehicle_conf:{conf}")

            if element_id:
                if element_id == primary_id:
                    found_primary_element = True
                    logger.debug(
                        f"primary element ECU defined, id:{primary_id}")

                    conf["publicKey"] = public_key
                    VEHICLE_CONF_TARGET_ID.validate(conf.get(VEHICLE_CONF_TARGET_ID.key))
                    VEHICLE_CONF_PUBLIC_KEY_ID.validate(conf.get(VEHICLE_CONF_PUBLIC_KEY_ID.key))

            VEHICLE_CONF_PART_NUM.validate(conf.get(VEHICLE_CONF_PART_NUM.key))

            soft_infos = conf.get(VEHICLE_CONF_SOFTWAREINFOS.key)
            if soft_infos:
                for soft_info in soft_infos:
                    VEHICLE_CONF_PRODUCT_NUMBER.validate(soft_info.get(VEHICLE_CONF_PRODUCT_NUMBER.key))
                    VEHICLE_CONF_SUBTARGET_ID.validate(soft_info.get(VEHICLE_CONF_SUBTARGET_ID.key))

        # primary element id is mandatory.
        if not found_primary_element:
            raise ValueError(
                f"There is no primary element in {VEHICLE_CONFS.key}")

    def validate_vehicle_json(self, vehicles, public_key):
        for vehicle in vehicles.get(VEHICLE_INFOS.key):
            VIN.validate(vehicle.get(VIN.key))
            VEHICLE_MAKE.validate(vehicle.get(VEHICLE_MAKE.key))
            VEHICLE_MODEL.validate(vehicle.get(VEHICLE_MODEL.key))
            VEHICLE_TRIM.validate(vehicle.get(VEHICLE_TRIM.key))
            VEHICLE_MODELYEAR.validate(vehicle.get(VEHICLE_MODELYEAR.key))
            REGION.validate(vehicle.get(REGION.key))
            EVENTID.validate(vehicle.get(EVENTID.key))

            primary_id = vehicle.get(PRIMARY_ELEMENTID.key)
            PRIMARY_ELEMENTID.validate(primary_id)

            v_confs = vehicle.get(VEHICLE_CONFS.key)
            VEHICLE_CONFS.validate(v_confs)

            # optional parameters
            VEHICLE_COLOR.validate(vehicle.get(VEHICLE_COLOR.key))
            EVENT_TYPE.validate(vehicle.get(EVENT_TYPE.key))
            PHONE_NUMBER.validate(vehicle.get(PHONE_NUMBER.key))

            self.__validate_vehicle_conf(v_confs, primary_id, public_key)

            self.validate_rxswins(vehicle.get("rxswins"))

    def validate_components(self, components: List):
        if components is None or len(components) == 0:
            return
        for comp in components:
            comp_id = comp.get(COMPONENTS_JSON_ID.key)
            COMPONENTS_JSON_ID.validate(comp_id)
            if type(comp_id) is not int:
                raise ValueError(
                    f"{COMPONENTS_JSON_ID.key} value shall be integer")

            # soft info is optional
            soft_infos = comp.get(COMPONENTS_JSON_SOFT_INFO.key)
            COMPONENTS_JSON_SOFT_INFO.validate(soft_infos)
            for soft_info in soft_infos:
                if soft_info is not None:
                    COMPONENTS_JSON_PRODUCT_NUMBER.validate(soft_info.get(COMPONENTS_JSON_PRODUCT_NUMBER.key))
                    COMPONENTS_JSON_SUBTARGET_ID.validate(soft_info.get(COMPONENTS_JSON_SUBTARGET_ID.key))
                    repro_method = soft_info.get(
                        COMPONENTS_JSON_REPRO_DELIVERY.key)
                    COMPONENTS_JSON_REPRO_DELIVERY.validate(repro_method)
                    if repro_method not in REPRO_DELIVERY_METHOD_CAND:
                        raise ValueError(
                            f"{COMPONENTS_JSON_REPRO_DELIVERY.key} shall be in {REPRO_DELIVERY_METHOD_CAND}.")

                    COMPONENTS_JSON_NOTE.validate(soft_info.get(COMPONENTS_JSON_NOTE.key))

    def validate_system_info(self, system_infos):
        for system_info in system_infos:
            result_sys_id = system_info.get(SYSTEM_INFORMATION_RESULT_SYSID.key)
            SYSTEM_INFORMATION_RESULT_SYSID.validate(result_sys_id)

            starts = system_info.get(SYSTEM_INFORMATION_STARTS.key)
            SYSTEM_INFORMATION_STARTS.validate(starts)

            for start in starts:
                start_sys_id = start.get(SYSTEM_INFORMATION_START_SYSID.key)
                SYSTEM_INFORMATION_START_SYSID.validate(start_sys_id)

                sub_target_repros = start.get(
                    SYSTEM_INFORMATION_SUBTARGET_REPROS.key)
                SYSTEM_INFORMATION_SUBTARGET_REPROS.validate(sub_target_repros)

                for repro in sub_target_repros:
                    element_conf_sw_id = repro.get(
                        SYSTEM_INFORMATION_ELEMENT_CONFIG_SW.key)
                    SYSTEM_INFORMATION_ELEMENT_CONFIG_SW.validate(element_conf_sw_id)
                    repro_type = repro.get(SYSTEM_INFORMATION_REPRO_TYPE.key)
                    SYSTEM_INFORMATION_REPRO_TYPE.validate(repro_type)
                    if repro_type not in SYSTEM_INFORMATION_REPRO_TYPE_LIST:
                        raise ValueError(
                            f"f{SYSTEM_INFORMATION_REPRO_TYPE.key} shall be defined and in {SYSTEM_INFORMATION_REPRO_TYPE_LIST}")

    def validate_modifications_json(self, mods_json):
        logger.debug(f"change event modifications json:{mods_json}")
        modifications = mods_json.get(CHANGE_EVENT_MODIFICATION.key)
        CHANGE_EVENT_MODIFICATION.validate(modifications)
        self.validate_system_info(modifications)

    def validate_rxswins(self, input_rxswins: List):
        num_str = sum([len(rxswin_str) for rxswin_str in input_rxswins])
        if (num_str > 32 / 2 - len(input_rxswins)):
            raise ValueError(
                f"rxswins should be less than 32 byte length after hex conversion.")

    def validate_upload_part_num(self, num_of_part: int):
        if (num_of_part < 1 or MULTI_UPLOAD_MAX_PART_NUM < num_of_part):
            raise ValueError(
                f"Number of part for multi-part upload should be within 1 to {MULTI_UPLOAD_MAX_PART_NUM}.")

    def validate_upload_part_size(self, part_size: int):
        if (part_size < MULTI_UPLOAD_MIN_PART_SIZE or MULTI_UPLOAD_MAX_PART_SIZE < part_size):
            raise ValueError(
                f"Part size for multi-part upload should be within {MULTI_UPLOAD_MIN_PART_SIZE} to {MULTI_UPLOAD_MAX_PART_SIZE} byte.")

    def parse_rxswins(self, input_rxswins: List):
        ret = []
        if input_rxswins is not None:
            err_msg = "RXSWIN should includes one and only one space character and not need to add escaped character input:"
            if len(input_rxswins) % 2 != 0:
                raise ValueError(
                    f"{err_msg}{input_rxswins}")
            for i in range(0, len(input_rxswins), 2):
                # already escaped but it is not acceptable
                if ' ' in input_rxswins[i] or ' ' in input_rxswins[i + 1]:
                    raise ValueError(
                        f"{err_msg}{input_rxswins}")

                ret.append(' '.join(
                    [input_rxswins[i], input_rxswins[i + 1]]))
        return ret

    def vsms_admin__vehicle_type__region__create(self,
                                                 name):
        body = {}
        body["name"] = name
        logger.debug(f"{body}")
        ret = self.post("vsms/admin/api/1.0/vehicle/type/region",
                        json=body,
                        auth=self.access_token)
        return ret

    def vsms_admin__vehicle_type__region__search(self,
                                                 name):
        params = {}
        if name is not None:
            params["name"] = name
        logger.debug(f"{params}")
        ret = self.get("vsms/admin/api/1.0/vehicle/type/regions",
                       params=params,
                       auth=self.access_token)
        return ret

    def vsms_admin__vehicle_type__region__delete(self,
                                                 id):
        ret = self.delete(f"vsms/admin/api/1.0/vehicle/type/region/{id}",
                          auth=self.access_token)
        return ret

    def vsms_admin__vehicle_type__delete(self,
                                         model, maker):
        params = {}
        if model:
            params['model'] = model
        if maker:
            params['make'] = maker
        logger.debug(f"{params}")
        ret = self.delete("vsms/admin/api/1.0/vehicle/type",
                          params=params,
                          auth=self.access_token)
        return ret

    def vsms_admin__subsystem__create(self, name: str,
                                      system_check: bool,
                                      rxswins: List,
                                      subsystem_id: str,
                                      vtype_ids: List):
        # 5.1.1. Create Subsystem
        if len(subsystem_id) > 24:
            raise ValueError(
                f"sys_id shall be smaller than 24 length. sysId:{subsystem_id}")

        ss_config = {
            'sysId': subsystem_id,  # ex. 'wa_sys63_pri_a'
        }
        if rxswins:
            ss_config['rxswins'] = rxswins
        if vtype_ids:
            ss_config['vehicleTypeIds'] = vtype_ids

        json = {
            'name': name,  # ex. 'wa_sys63_pri'
            'systemCheck': system_check,  # to be true?
            'subsystemConfiguration': ss_config,
        }
        logger.debug(f"{json}")
        ret = self.post('vsms/admin/api/1.0/subsystem',
                        json=json,
                        auth=self.access_token)
        logger.debug(f"-> {ret}")
        return ret

    def vsms_admin__subsystem__get(self, id: str):
        # 5.1.2. Get Subsystem List
        ret = self.get(f"vsms/admin/api/1.0/subsystem/{id}",
                       auth=self.access_token)
        return ret

    def vsms_admin__subsystem_delete(self, id):
        ret = self.delete(f"vsms/admin/api/1.0/subsystem/{id}",
                          auth=self.access_token)
        return ret

    def vsms_admin__hwcomponent__create(self, target_id, ecu_name, repro_platform,
                                        part_number=None,
                                        phys_addr=None,
                                        rom_partirion=None):
        _ = oct(int(target_id, 8))
        if len(ecu_name) > 50:
            raise ValueError(
                f"ecu name shall be smaller than 50 length. ecu_name:{ecu_name}")
        if phys_addr and len(phys_addr) > 24:
            raise ValueError(
                f"phys_address shall be smaller than 24 length. physical_addr:{phys_addr}")
        json = {
            "targetId": target_id,
            "ecuName": ecu_name,
            "reproPlatform": repro_platform
        }
        if part_number:
            json['partNumber'] = part_number
        if phys_addr:
            json['physicalAddress'] = phys_addr
        if rom_partirion:
            json['romPartition'] = rom_partirion
        logger.debug(f"{json}")
        ret = self.post('vsms/admin/api/1.0/hwcomponent',
                        json=json,
                        auth=self.access_token)
        logger.debug(f"-> {ret}")
        return ret

    def vsms_admin__subsystem_configuration__create(self, subsys_id: int, old_sys_id: str, new_sys_id: str, rxswins: List[str], components=None):
        # 5.1.10 Create Subsystem Configuration
        logger.debug(
            f"update subsystem-configuration with subsystem, subsystem_id:{subsys_id} old_sysid:{old_sys_id} new_sys_id:{new_sys_id}")
        body = {}
        body["sysId"] = new_sys_id
        if rxswins is not None:
            body["rxswins"] = rxswins
        if old_sys_id is not None:
            body["compareSysId"] = old_sys_id

        if components is not None:
            body["components"] = components

        logger.debug(f"{body}")
        ret = self.post((f"vsms/admin/api/1.0/subsystem/" +
                        f"{subsys_id}/subsystem-configuration"),
                        auth=self.access_token,
                        json=body)
        logger.debug(f"=>{ret}")
        return ret

    def vsms_admin__subsystem_configuration__edit(self, ss_config_id: int,
                                                  sys_id: str = None,
                                                  rxswins: List = None,
                                                  components: List = None):
        # 5.1.5. Update Subsystem Configuration
        json = {}
        if sys_id:
            json['sysId'] = sys_id
        if components:
            json['components'] = components
        if rxswins:
            json['rxswins'] = rxswins
        logger.debug(f"ss_config_id={ss_config_id}, {json}")
        ret = self.put(("vsms/admin/api/1.0/subsystem-configuration/" +
                        f"{ss_config_id}"),
                       json=json,
                       auth=self.access_token)
        logger.debug(f"-> {ret}")
        return ret

    def vsms_admin__vehicle_design__create(self, vehicle_type_ids: List[int],
                                           status: str,
                                           subsystems: List = None, prev_vehicle_design_id: str = None,
                                           prev_rxswin: str = None, new_rxswin: str = None, same_rxswin_system_name: str = None, updated=None):
        RXSWIN_UPDATE_INFO_KEY = "systemRxswinUpdateInfos"
        NEW_RXSWIN_KEY = "newRxswin"
        PREV_RXSWIN_KEY = "prevRxswin"
        RXSWIN_SYSTEM_NAME_KEY = "sameRxswinSystemName"
        UPDATED_KEY = "updated"
        # 5.3.1. Create Vehicle Design Information
        #
        # - status: "DRAFT"/"PENDING"
        json = {
            "vehicleTypeIds": vehicle_type_ids,
            "status": status,
        }
        if subsystems:
            json['subsystems'] = subsystems

        if prev_vehicle_design_id:
            json['prevVehicleDesignInformationId'] = prev_vehicle_design_id

        if prev_rxswin or new_rxswin or same_rxswin_system_name or updated:
            rxswin_update_info = {}
            if prev_rxswin:
                rxswin_update_info[PREV_RXSWIN_KEY] = prev_rxswin
            if new_rxswin:
                rxswin_update_info[NEW_RXSWIN_KEY] = new_rxswin
            if same_rxswin_system_name:
                rxswin_update_info[RXSWIN_SYSTEM_NAME_KEY] = same_rxswin_system_name
            if updated:
                rxswin_update_info[UPDATED_KEY] = updated
            json[RXSWIN_UPDATE_INFO_KEY] = rxswin_update_info

        ret = self.post("vsms/admin/api/1.0/vehicle-design",
                        json=json,
                        auth=self.access_token)
        logger.debug(f"{ret}")
        return ret

    def vsms_admin__change_event__create(self, change_event_id: str, name: str, desc: str, type: str,
                                         pkg_id: str = None, authority_report_id: str = None,
                                         reason_class: int = 9, reason_content: str = None, status: str = None, dependent_sysids: List = None, modifications=None):
        json = {}
        logger.debug(
            f"change event creation, parameters; change_event_id:{change_event_id}")
        json["changeEventId"] = change_event_id
        json["name"] = name
        json["description"] = desc
        json["reasonClass"] = reason_class
        json["type"] = type

        if pkg_id:
            json["packageId"] = pkg_id
        if authority_report_id:
            json["authorityReportId"] = authority_report_id
        if reason_content:
            json["reasonContent"] = reason_content
        if status:
            json["status"] = status
        if dependent_sysids:
            json["dependentSysIds"] = dependent_sysids
        # already validated by caller
        if modifications:
            json["modifications"] = modifications

        logger.debug(f"change event post body {json}")
        # 8.3 create change event
        ret = self.post("vsms/admin/api/1.0/change-event",
                        json=json, auth=self.access_token)
        logger.debug(f"=>{ret}")
        return ret

    def vsms_admin__change_event__update(self, id: str, change_event_id: str, name: str, desc: str,
                                         pkg_id: str = None, authority_report_id: str = None,
                                         reason_class: int = 9, dependent_sysids: List = None, modifications=None):
        json = {}
        logger.debug(
            f"change event updated, parameters; change_event_id:{change_event_id}")
        json["changeEventId"] = change_event_id
        json["name"] = name
        json["description"] = desc
        json["reasonClass"] = reason_class

        if pkg_id:
            json["packageId"] = pkg_id
        if authority_report_id:
            json["authorityReportId"] = authority_report_id
        if dependent_sysids:
            json["dependentSysIds"] = dependent_sysids
        # already validated by caller
        if modifications:
            json["modifications"] = modifications

        logger.debug(f"change event post body {json}")
        # update change event
        ret = self.put(f"vsms/admin/api/1.0/change-event/{id}",
                       json=json, auth=self.access_token)
        logger.debug(f"=>{ret}")
        return ret

    def vsms_admin__change_event__delete(self, id):
        ret = self.delete(f"vsms/admin/api/1.0/change-event/{id}",
                          auth=self.access_token)
        return ret

    def vsms_admin__subsystem__diff(self, systems, depend_sysids: List = None):
        json = {}
        json["systems"] = systems
        if depend_sysids:
            json["dependentSysIds"] = depend_sysids
        ret = self.post("vsms/admin/api/1.0/subsystem/diff",
                        json, auth=self.access_token)
        return ret

    def vsms_admin__vdesign__delete(self, id):
        ret = self.delete(f"vsms/admin/api/1.0/vehicle-design/{id}",
                          auth=self.access_token)
        return ret

    def vsms_admin__change_event_package__delete(self, id):
        ret = self.delete(f"vsms/admin/api/1.0/change-event-package/{id}",
                          auth=self.access_token)
        return ret

    def factory_feed__get(self, vin: str):
        # 3.2 search vehicles
        params = {}
        # case sensitive complete match or substring
        params["vins"] = vin

        logger.debug(f"GET factoryfeed/api/1.0/vehicles")
        ret = self.get("factoryfeed/api/1.0/vehicles",
                       params=params, auth=self.access_token)
        return ret

    def factory_feed__delete(self, vin, eventId, is_confirm):
        # delete vehicle info
        DELETE_VEHICLE_INFOS_KEY = "deleteVehicleInfos"

        logger.debug(f"DELETE factoryfeed vehicle")
        body = {}
        # TODO: support multi delete.
        body[DELETE_VEHICLE_INFOS_KEY] = [{
            "vin": vin,
            "eventId": eventId,
            "confirmedDeletion": is_confirm
        }]

        # from API document, PUT is correct not DELETE
        ret = self.put("factoryfeed/api/1.0/vehicles",
                       body,
                       auth=self.access_token)
        logger.debug(f"=>{ret}")
        return ret

    def vsms_admin__initiate_multi_part_upload(self, id: str, package_file_name: str, file_type: str, md5_check_sums: List, file_hash_value: str = None, target_id: str = None):
        json = {}
        json["contentType"] = "application/binary"
        json["uploadParts"] = [{"number": i + 1, "md5CheckSum": value} for i, value in enumerate(md5_check_sums)]
        json["fileName"] = package_file_name
        json["fileType"] = file_type
        if file_hash_value and len(file_hash_value) > 0:
            json["fileHashValue"] = file_hash_value
        if target_id and len(target_id) > 0:
            json["targetId"] = target_id
        logger.debug(f"mulit part upload initiate URL body:{json}")
        ret = self.put(
            f"vsms/admin/api/1.0/change-event-package/{id}/upload/initiate", json=json, auth=self.access_token)
        return ret

    def vsms_admin__upload_part(self, upload_url: str, content: str, md5sum: str):
        # upload to presigned URL.
        headers = {}
        # if below headers are attached to request, 403 error occurred because of non-signed headers.
        headers["Content-Type"] = "application/binary"
        headers["Content-Length"] = str(len(content))
        headers["Content-MD5"] = md5sum
        headers = dict(headers)
        with requests.request("PUT", upload_url, data=content,
                              headers=headers,
                              proxies=self.proxies,
                              verify=self.verity_setver_tls_cert,) as resp:
            logger.debug(f"status_code={resp.status_code} ({upload_url})")
            logger.debug(f"{resp.text}")
            if resp.status_code in (200, 201):
                # 'Ok'/'created'
                return resp.headers
            raise ValueError(f"{resp.status_code, resp.text}")

    def vsms_admin__complete_multi_part_upload(self, upload_context: str, part_ids: List):
        json = {}
        json["uploadContext"] = upload_context
        json["uploadParts"] = [{"number": i + 1, "id": id} for i, id in enumerate(part_ids)]
        logger.debug(f"complete multi part upload URL body:{json}")
        ret = self.post(
            "vsms/admin/api/1.0/package/upload/complete", json=json, auth=self.access_token)
        logger.debug(f"=>{ret}")
        return ret

    def open_upload_file(self, file_path: str, file_type: str):
        if file_type == VSPA_FILETYPE:
            tmp = tempfile.TemporaryFile()
            with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as archive:
                archive.write(file_path, os.path.basename(file_path))
            tmp.seek(0)
            return tmp
        else:
            return open(file_path, "rb")

    def __get_base64_md5_list(self, file, size):
        base64_md5_list = []
        while True:
            content = file.read(size)
            if len(content) == 0:
                break  # EOF
            md5_hash = hashlib.md5(content).digest()
            base64_md5 = base64.b64encode(md5_hash).decode("utf-8")
            base64_md5_list.append(base64_md5)
        return base64_md5_list

    def get_base64_md5_list(self, file_path: str, file_type: str, size: int = -1):
        file = self.open_upload_file(file_path, file_type)
        base64_md5_list = self.__get_base64_md5_list(file, size)
        file.close()
        return base64_md5_list

    def vsms_admin__change_event_package__get_upload_info(self, id: str, package_file_path: str, package_file_name: str, file_type: str):
        json = {}
        json["fileName"] = package_file_name
        json["fileType"] = file_type
        logger.debug(f"get upload URL body:{json}")
        ret = self.put(
            f"vsms/admin/api/1.0/change-event-package/{id}/upload", json=json, auth=self.access_token)
        return ret

    def vsms_admin__change_event_package__get_download_info(self, id: str, file_type: str):
        params = {}
        params["fileType"] = file_type

        ret = self.get(
            f"vsms/admin/api/1.0/change-event-package/{id}/download-path", params=params, auth=self.access_token)
        return ret

    def vsms_admin__change_event_package__upload_package(self, url: str, file_path: str, file_type: str):
        # TODO for large size file, hash should be calculated from split file, and use multiple upload
        # calc md5
        content = None
        md5_hash = None

        if file_type == VSPA_FILETYPE:
            with tempfile.TemporaryFile() as tmp:
                with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED) as archive:
                    archive.write(file_path)
                tmp.seek(0)

                content = tmp.read()
                md5_hash = hashlib.md5(content).hexdigest()
        else:
            with open(file_path, "rb") as file:
                content = file.read()
                md5_hash = hashlib.md5(content).hexdigest()

        logger.debug(f"upload URL:{url} md5 hash:{md5_hash}")

        # for debugging purpose
        # import http
        # http.client.HTTPConnection.debuglevel=1
        if content:
            # upload to presigned URL.
            headers = {}
            # if below headers are attached to request, 403 error occurred because of non-signed headers.
            headers["Content-Type"] = "application/octet-stream"
            # headers["Content-MD5"]=base64.b64encode(bytes.fromhex(md5_hash))
            headers = dict(headers)
            with requests.request("PUT", url, data=content,
                                  headers=headers,
                                  proxies=self.proxies,
                                  verify=self.verity_setver_tls_cert,
                                  timeout=self.TIMEOUT_SECS
                                  ) as resp:
                logger.debug(f"status_code={resp.status_code} ({url})")
                logger.debug(f"{resp.text}")
                if resp.status_code in (200, 201):
                    # 'Ok'/'created'
                    if resp.text:
                        got = resp.json()
                        logging.debug(got)
                        return got
                    else:
                        return {}
                raise ValueError(f"{resp.status_code, url}")

    def download_file(self, url: str, out: str, file_type: str):
        # TODO: file may be too large not on memory. streaming download is better.
        urldata = requests.get(url).content
        if file_type == VSPA_FILETYPE:
            with tempfile.TemporaryFile() as tmp:
                tmp.write(urldata)
                with zipfile.ZipFile(tmp, "r") as archive:
                    file_list = archive.namelist()
                    if len(file_list) != 1:
                        raise ValueError(f"cannot be download a package file because uploaded file includes other files in addition to package file itself. file_list:{file_list}")
                    with archive.open(file_list[0]) as zf:
                        with open(out, mode="wb") as f:
                            f.write(zf.read())

        else:
            with open(out, mode="wb") as f:
                f.write(urldata)

    def vsms_admin__campaign__delete(self, id):
        ret = self.delete(f"vsms/admin/api/1.0/campaign/{id}",
                          auth=self.access_token)
        return ret

    def vsms_admin__campaign__modify_change_event(self, campaign_id: str, change_events: List[int]):
        # 4.3.42 modify campaign change events
        # change events list of change events c
        # change events is Tuple (id, priority)
        body = {}
        body["changeEvents"] = []
        if change_events:
            for i in range(0, len(change_events), 2):
                body["changeEvents"].append({
                    "id": change_events[i],
                    "priority": change_events[i + 1]
                })

        logger.debug(f"modify campaign's change event body {body}")
        ret = self.put(
            f"vsms/admin/api/1.0/campaign/{campaign_id}/change-event", json=body, auth=self.access_token)

        return ret

    def vsms_admin__campaign__modify_vehicle_types(self, campaign_id: str, vehicle_types: List[int]):
        # 4.3.42 modify campaign vehicle types
        body = {}
        body["vehicleTypeIds"] = []
        for v in vehicle_types:
            body["vehicleTypeIds"].append(v)

        logger.debug(f"modify campaign's vehicle_types body {body}")
        ret = self.put(
            f"vsms/admin/api/1.0/campaign/{campaign_id}/vehicle_types", json=body, auth=self.access_token)

        return ret

    def vsms_admin__campaign__add_vins(self, campaign_id: str, vins: List[str]):
        # 4.3.18 modify campaign add vins manually

        logger.debug(f"modify campaign add vins {vins}")
        ret = self.post(
            f"vsms/admin/api/1.0/campaign/{campaign_id}/vins", json=vins, auth=self.access_token)

        return ret

    def vsms_admin__campaign__add_localized_messages(self, campaign_id: str, file_name: str, file_obj: bytes):
        logger.debug(f"add localized message {file_name} to campaign")
        ret = self.put(
            f"vsms/admin/api/1.0/campaign/{campaign_id}/messages?fileName={file_name}", data=file_obj, auth=self.access_token)

        return ret

    def help_desk__systemevents__get(self, vin: str):
        params = {}
        params["limit"] = 10
        params["page"] = 1
        ret = self.get(
            f"helpdesk/admin/api/1.0/vehicle/{vin}/system_events", params=params, auth=self.access_token)

        return ret

    def help_desk__current_config__get(self, vin: str):
        ret = self.get(
            f"helpdesk/admin/api/1.0/vehicle/{vin}/current_config", params={}, auth=self.access_token)
        return ret

    def help_desk__campaign_updates__get(self, vin: str):
        params = {}
        params["limit"] = 10
        params["page"] = 1
        ret = self.get(
            f"helpdesk/admin/api/1.0/vehicle/{vin}/campaign_updates", params=params, auth=self.access_token)
        return ret
