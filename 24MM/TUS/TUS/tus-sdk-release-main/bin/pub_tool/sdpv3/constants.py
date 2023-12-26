"""
TMC CONFIDENTIAL
$TUSLibId$
Copyright (C) 2022 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""

import dataclasses


@dataclasses.dataclass
class ConfigParam:
    key: str
    max_len: int = None
    is_mandatory: bool = False

    def validate(self, value):
        if self.is_mandatory and (not value or len(str(value)) == 0):
            raise ValueError(f"{self.key} should be defined.")
        if value and self.max_len and (len(str(value)) > self.max_len):
            raise ValueError(f"length of {self.key} should be less than {self.max_len}")


B2BPORTAL = "https://t-poc-r3-ap-northeast-1-b2b.t-poc-r3.com/"

COMPONENTS_JSON_ID = ConfigParam("id")
COMPONENTS_JSON_SOFT_INFO = ConfigParam("softwareInfos")
COMPONENTS_JSON_PRODUCT_NUMBER = ConfigParam("productNumber", 128, is_mandatory=True)
COMPONENTS_JSON_SUBTARGET_ID = ConfigParam("subTargetId", 5)
COMPONENTS_JSON_REPRO_DELIVERY = ConfigParam("reproDeliveryMethod", is_mandatory=True)
COMPONENTS_JSON_NOTE = ConfigParam("note", 200)
REPRO_DELIVERY_METHOD_CAND = ["storage", "streaming"]

COMPONENTS_JSON = f'''{{
    "components":[
        {{
            "{COMPONENTS_JSON_ID.key}":0,
            "{COMPONENTS_JSON_SOFT_INFO.key}":[{{
                "{COMPONENTS_JSON_PRODUCT_NUMBER.key}":"",
                "{COMPONENTS_JSON_SUBTARGET_ID.key}":"",
                "{COMPONENTS_JSON_REPRO_DELIVERY.key}":"{REPRO_DELIVERY_METHOD_CAND[1]}",
                "{COMPONENTS_JSON_NOTE.key}":""
            }}]
        }}
    ]}}'''


VEHICLE_INFOS = ConfigParam("vehicleInfos")
VIN = ConfigParam("vin", 17, is_mandatory=True)
VEHICLE_MAKE = ConfigParam("vehicleMake", 50, is_mandatory=True)
VEHICLE_MODEL = ConfigParam("vehicleModel", 50, is_mandatory=True)
VEHICLE_TRIM = ConfigParam("vehicleTrim", 50, is_mandatory=True)
VEHICLE_MODELYEAR = ConfigParam("modelYear", 9999, is_mandatory=True)
REGION = ConfigParam("region", 255, is_mandatory=True)
RXSWINS = ConfigParam("rxswins")
EVENTID = ConfigParam("eventId", 255, is_mandatory=True)
OTAENABLED = ConfigParam("otaEnabled")
PRIMARY_ELEMENTID = ConfigParam("primaryElementId", 24, is_mandatory=True)
PHONE_NUMBER = ConfigParam("phoneNumber", 16)
EQUIPPED_SYSTEM = ConfigParam("equippedSystems")
VEHICLE_MANUFACT_DATE = ConfigParam("vehicleManufactureDate")
VEHICLE_COLOR = ConfigParam("vehicleColor", 20)
EVENT_TYPE = ConfigParam("eventType", 20)

VEHICLE_CONFS = ConfigParam("vehicleConfigurations", is_mandatory=True)
VEHICLE_CONF_ELEMENT_ID = ConfigParam("elementId", 24)
VEHICLE_CONF_PART_NUM = ConfigParam("partNumber", 16)
VEHICLE_CONF_TARGET_ID = ConfigParam("targetId", 5, is_mandatory=True)
VEHICLE_CONF_PUBLIC_KEY_ID = ConfigParam("publicKeyId", 255, is_mandatory=True)
VEHICLE_CONF_KEY_TYPE = ConfigParam("keyType")
VEHICLE_CONF_SOFTWAREINFOS = ConfigParam(COMPONENTS_JSON_SOFT_INFO.key)
VEHICLE_CONF_PRODUCT_NUMBER = ConfigParam(COMPONENTS_JSON_PRODUCT_NUMBER.key, 255, is_mandatory=True)
VEHICLE_CONF_SUBTARGET_ID = ConfigParam(COMPONENTS_JSON_SUBTARGET_ID.key, 5, is_mandatory=True)

MULTI_UPLOAD_MAX_PART_NUM = 10000
MULTI_UPLOAD_MAX_PART_SIZE = 5368709120  # 5GiB
MULTI_UPLOAD_MIN_PART_SIZE = 5242880  # 5MiB
MULTI_UPLOAD_DEFAULT_PART_SIZE = 5242880  # 5MiB

# not direct into json file
VEHICLE_CONF_PUBLIC_KEY = "publicKey"

FACTORY_FEED_JSON = f'''{{
    "{VEHICLE_INFOS.key}":[{{
        "{VIN.key}":"",
        "{VEHICLE_MAKE.key}":"",
        "{VEHICLE_MODEL.key}":"",
        "{VEHICLE_TRIM.key}":"",
        "{VEHICLE_MODELYEAR.key}":2024,
        "{REGION.key}":"Japan",
        "{RXSWINS.key}":[""],
        "{EVENTID.key}":"",
        "{OTAENABLED.key}":true,
        "{PHONE_NUMBER.key}":"",
        "{PRIMARY_ELEMENTID.key}":"",
        "{VEHICLE_MANUFACT_DATE.key}":"2022-01-01",
        "{VEHICLE_COLOR.key}":"black",
        "{EVENT_TYPE.key}":"BUILT",
        "{EQUIPPED_SYSTEM.key}":[""],
        "{VEHICLE_CONFS.key}":[{{
            "{VEHICLE_CONF_ELEMENT_ID.key}":"",
            "{VEHICLE_CONF_PART_NUM.key}":"",
            "{VEHICLE_CONF_TARGET_ID.key}":"",
            "{VEHICLE_CONF_KEY_TYPE.key}":"",
            "{VEHICLE_CONF_PUBLIC_KEY_ID.key}":"",
            "{VEHICLE_CONF_SOFTWAREINFOS.key}":[{{
                "{VEHICLE_CONF_PRODUCT_NUMBER.key}":"",
                "{VEHICLE_CONF_SUBTARGET_ID.key}":""
            }}]
        }}]
    }}]
}}'''

CHANGE_EVENT_MODIFICATION = ConfigParam("modifications", is_mandatory=True)
SYSTEM_INFORMATION_RESULT_SYSID = ConfigParam("resultSysId", is_mandatory=True)
SYSTEM_INFORMATION_STARTS = ConfigParam("starts", is_mandatory=True)
SYSTEM_INFORMATION_START_SYSID = ConfigParam("startSysId", is_mandatory=True)
SYSTEM_INFORMATION_SUBTARGET_REPROS = ConfigParam("subTargetReproTypes", is_mandatory=True)
SYSTEM_INFORMATION_ELEMENT_CONFIG_SW = ConfigParam("elementConfigSwId", is_mandatory=True)
SYSTEM_INFORMATION_REPRO_TYPE = ConfigParam("reproType", is_mandatory=True)
SYSTEM_INFORMATION_REPRO_TYPE_LIST = ["full", "incremental"]

# used by subsysdiff generation and change event
SYSTEM_INFORMATION_JSON = f'''{{
            "{SYSTEM_INFORMATION_STARTS.key}":[
                {{
                    "{SYSTEM_INFORMATION_START_SYSID.key}":"",
                    "{SYSTEM_INFORMATION_SUBTARGET_REPROS.key}":[
                        {{
                            "{SYSTEM_INFORMATION_ELEMENT_CONFIG_SW.key}":0,
                            "{SYSTEM_INFORMATION_REPRO_TYPE.key}":"{SYSTEM_INFORMATION_REPRO_TYPE_LIST[0]}"
                        }}
                    ]
                }}
            ],
            "{SYSTEM_INFORMATION_RESULT_SYSID.key}":""
        }}'''

CHANGE_EVENT_MODIFICATION_JSON = f'''{{
    "{CHANGE_EVENT_MODIFICATION.key}":[
        {SYSTEM_INFORMATION_JSON}
    ]
}}'''

SUBSYSDIFF_SYSTEMS = ConfigParam("systems", is_mandatory=True)
SUBSYSDIFF_GENERATION_JSON = f'''{{
    "{SUBSYSDIFF_SYSTEMS.key}": [
        {SYSTEM_INFORMATION_JSON}
    ]
}}'''

VSPA_FILETYPE = "VSPA"
REPROMETA_FILETYPE = "REPRO_METADATA"
DLMETA_FILETYPE = "DOWNLOAD_METADATA"
FILETYPE_LIST = [VSPA_FILETYPE, REPROMETA_FILETYPE, DLMETA_FILETYPE]

# keys to print when search-vehicle command is called
SEARCH_VEHICLE_KEYS = ["vin", "vehicleMake", "vehicleModel", "vehicleManufactureDate", "region", "modelYear", "primaryElementId"]
