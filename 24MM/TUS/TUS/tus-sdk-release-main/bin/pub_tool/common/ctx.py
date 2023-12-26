import sys
import requests
import logging
from typing import List

logger = logging.getLogger(__name__)


class CTX:
    TIMEOUT_SECS = 5

    def __init__(self, baseurl, username, passwd, proxies=None):
        self.baseurl = baseurl

        self.verity_setver_tls_cert = True

        self.username = username
        # for future use
        self.proxies = proxies

        # B2B portal account's password. no to be passed as a cmd-arg
        password = passwd

        self.access_token = None
        self.refresh_token = None

        self.idm__login(self.username, password)

    def _make_url(self, rel):
        return self.baseurl + rel

    def request(self, method, rel,
                json=None, data=None, headers=None, params=None, auth=None):
        url = self._make_url(rel)
        if headers:
            headers = dict(headers)  # clone
        else:
            headers = {}
        headers['Content-Type'] = 'application/json'
        if data:
            headers['Content-Type'] = 'application/octet-stream'
        if auth:
            headers['Authorization'] = auth
        logger.debug(f"{url, headers, json}")
        rep = 1
        while rep >= 0:
            rep -= 1
            with requests.request(method, url,
                                  json=json,
                                  data=data,
                                  headers=headers,
                                  params=params,
                                  proxies=self.proxies,
                                  verify=self.verity_setver_tls_cert,
                                  timeout=self.TIMEOUT_SECS
                                  ) as resp:
                logger.debug(f"status_code={resp.status_code} ({url})")
                if resp.status_code in (200, 201, 202):
                    # 'Ok'/'created'
                    if resp.text:
                        got = resp.json()
                        logging.debug(got)
                        return got
                    else:
                        return {}
                if resp.status_code == 401:
                    # auth token exxpired?
                    if auth:
                        headers['Authorization'] = self.idm__refresh()
                        continue
                sdp_code = (resp.json() or {}).get('code')

                message = ""
                if sdp_code == 44123:
                    message = "sys_ids are duplicated. Please set a new value and try again. \n"
                message = message + \
                    f"(status_code, sdp_code, url) = ({resp.status_code}, {sdp_code}, {url})"

                logging.error(
                    "response status from server is not successful. Please check your input and the following details.")
                logging.error(f"{message}")
                sys.exit(-1)

        raise ValueError(f"{url, headers}")

    def post(self, rel, json, headers=None, auth=None):
        return self.request('POST', rel, json=json, headers=headers, auth=auth)

    def put(self, rel, json=None, data=None, headers=None, auth=None):
        return self.request('PUT', rel, json=json, data=data, headers=headers, auth=auth)

    def get(self, rel, headers=None, auth=None, params=None):
        return self.request('GET', rel, params=params, auth=auth)

    def delete(self, rel, headers=None, auth=None, params=None):
        return self.request('DELETE', rel, params=params, auth=auth)

    def __repr__(self):
        return f"<<CTX: username='{self.username}' {self.baseurl}>>"

    def idm__login(self, username, password):
        json = self.post("idm/api/1.0/login",
                         {
                             "username": username,
                             "password": password,
                         })
        if not json:
            raise ValueError("login failed")
        self.access_token = json['authToken']
        self.refresh_token = json['refreshToken']
        logger.debug(f"{self.access_token, self.refresh_token}")

    def idm__refresh(self):
        refresh_token = self.refresh_token
        if not refresh_token:
            raise ValueError("no refresh_token")
        logger.warn("try to refresh access_token")
        self.refresh_token = None  # inhibit refresh loop
        json = self.post("idm/api/1.0/refresh",
                         {
                             "refreshToken": refresh_token,
                         })
        if not json:
            raise ValueError("refresh failed")
        self.access_token = json['authToken']
        self.refresh_token = refresh_token
        logger.debug(self.access_token, self.refresh_token)
        return self.access_token

    def vsms_admin__vehicle_type__create(self, maker,
                                         model=None, trim=None,
                                         year: str = None, region=None):
        # 6.1.1. Create Vehicle Types
        new_type = {"makeCode": maker}
        if model:
            new_type["modelCode"] = model
        if trim:
            new_type["trimCode"] = trim
        if year:
            # Enter range of years or individual years by comma.
            # Use * to include future years.
            #  (i.e. 2007, 2008, 2010-2013, 2015-*)
            new_type["yearCode"] = year
        if region:
            new_type["region"] = region
        logger.debug(f"{new_type}")
        return self.post("vsms/admin/api/1.0/vehicle/type",
                         json=[new_type],
                         auth=self.access_token)

    def vsms_admin__vehicle_type__get(self,
                                      model=None, campaign=None, maker=None, trim=None, year=None,
                                      exact=False, limit=None):
        # 6.1.3. Get VehicleTypes
        #
        # - 'model':" to filter by "modelCode" (!! partial string match)
        # - 'campaignId': to get models the canpaign is used for
        #  returns list of
        #   {'id':, 'region':, 'inUse':, 'makeCode':, 'modelCode':,
        #    'trimCode':, 'yearCode':, 'selected':}
        params = {}
        if model:
            params['model'] = model
        if campaign:
            params['campaignId'] = campaign
        logger.debug(f"{params}")
        ret = self.get("vsms/admin/api/1.0/vehicle/type",
                       params=params,
                       auth=self.access_token)
        if exact and model:
            logger.debug(f"ret:{ret}")
            ret = [e for e in ret if e.get('modelCode') == str(model)]
            if maker is not None:
                # require to have the makeCode
                ret = [e for e in ret if e.get('makeCode') == str(maker)]
            if trim is not None:
                ret = [e for e in ret if e.get('trimCode') == str(trim)]
            if year is not None:
                ret = [e for e in ret if e.get('yearCode') == str(year)]
        return ret

    def vsms_admin__hwcomponent__get(self, subsys_ids: List[int] = None,
                                     name: str = None, exact: str = None):
        # 5.2.2. Search Hardware Components
        #
        # - subsystemIds:
        # - name: Part Number (ECU HW ID) or TargetId or ECU name
        # exact: 'partNumber'/ 'ecuName' / 'targetId'
        params = {'limit': 100}  # see 3.4. Pagination

        if name:
            params['hwSearch'] = name
        if subsys_ids:
            params['subsystem-id'] = subsys_ids
        logger.debug(f"{params}")
        ret = self.get("vsms/admin/api/1.0/hwcomponent",
                       params=params,
                       auth=self.access_token)
        logger.debug(f"-> {ret}")
        # TODO: currently do not support pagination
        # if ret['totalCount'] > params['limit']:
        # raise ValueError('todo: paginate?')
        if exact and name:
            filtered = []
            for r in ret['results']:
                if r['hardware'].get(exact) == name:
                    filtered.append(r)
                elif r['target'].get(exact) == name:
                    filtered.append(r)
            return filtered  # return [] if nothing was found
        return ret

    def vsms_admin__vehicle_design__get(self, vehicle_design_id: int):
        # 5.3.3. Get Vehicle Design Information
        params = {}
        ret = self.get("vsms/admin/api/1.0/vehicle-design/"
                       f"{vehicle_design_id}",
                       params=params,
                       auth=self.access_token)
        logger.debug(f"-> {ret}")
        return ret

    def vsms_admin__vehicle_design__approve(self, vehicle_design_id: int):
        # 5.3.5. Approve Vehicle Design Information
        ret = self.put("vsms/admin/api/1.0/vehicle-design/"
                       f"{vehicle_design_id}/approve",
                       json=None,
                       auth=self.access_token)
        logger.debug(f"-> {ret}")
        return ret

    def factory_feed__post(self, body):
        logger.debug(f"POST factoryfeed vehicle")

        vehicles = body.get("vehicleInfos")
        if vehicles is None:
            raise ValueError("invalid json file.")
        for vehicle in vehicles:
            if vehicle.get("vin") is None:
                raise ValueError("vin shall be 17 length string.")
            else:
                if len(vehicle.get("vin")) != 17:
                    raise ValueError("vin shall be 17 length string.")

        ret = self.post("factoryfeed/api/1.0/vehicles",
                        body, auth=self.access_token)
        logger.debug(f"=>{ret}")

        if ret.get("failure") != 0:
            raise ValueError("Some vehicles cannot be created. response :{}", ret)

        return ret

    def vsms_admin__change_event__submit(self, change_event_id: str):
        # 8.7 submit for approval
        ret = self.put(f"vsms/admin/api/1.0/change-event/{change_event_id}/submit",
                       json={},
                       auth=self.access_token)
        return ret

    def vsms_admin__change_event__approve(self, change_event_id: str):
        # 8.8 approve change event
        ret = self.put(f"vsms/admin/api/1.0/change-event/{change_event_id}/approve",
                       json={},
                       auth=self.access_token)
        return ret

    def vsms_admin__change_event__get(self, id: str):
        logger.debug(f"change event search")

        ret = self.get(f"vsms/admin/api/1.0/change-event/{id}",
                       auth=self.access_token)
        logger.debug(f"=>{ret}")
        return ret

    def vsms_admin__change_event_package__create(self, package_name: str, diff_json: str, file_name: str):
        # 9.6 Create change event package
        if len(package_name) > 255:
            raise ValueError(f"name should be less than 255 length.")
        if len(file_name) > 255:
            raise ValueError(f"diff name shall be less than 255 length.")

        json = {}
        json["name"] = package_name
        json["diffJson"] = diff_json
        json["diffName"] = file_name
        logger.debug(f"change event package creation body:{json}")
        ret = self.post("vsms/admin/api/1.0/change-event-package",
                        json=json, auth=self.access_token)
        return ret

    def vsms_admin__change_event_package__update(self, id: str, status: str, name: str = None, diffjson: str = None, diffname: str = None):
        json = {}
        if status:
            json["status"] = status
        if name:
            if len(name) > 255:
                raise ValueError(f"name should be less than 255 length.")
            json["name"] = name
        if diffjson:
            json["diffJson"] = diffjson
        if diffname:
            if len(diffname) > 255:
                raise ValueError(f"diff name shall be less than 255 length.")
            json["diffName"] = diffname
        ret = self.put(
            f"vsms/admin/api/1.0/change-event-package/{id}", json=json, auth=self.access_token)
        return ret

    def vsms_admin__change_event_package__approve(self, id: str):
        ret = self.put(
            f"vsms/admin/api/1.0/change-event-package/{id}/approved", json={}, auth=self.access_token)
        return ret

    def vsms_admin__change_event_package__get(self, id: str):
        logger.debug(f"change event get")

        ret = self.get(f"vsms/admin/api/1.0/change-event-package/{id}",
                       auth=self.access_token)
        logger.debug(f"=>{ret}")
        return ret

    def vsms_admin__campaign__create(self, name: str, description: str, type: str):
        body = {}
        body["name"] = name
        body["description"] = name
        body["campaignType"] = type
        ret = self.post("vsms/admin/api/1.0/campaign",
                        json=body, auth=self.access_token)

        # by default, newly created campaign combined to all vehicle types???
        return ret

    def vsms_admin__package__process(self, id: str):
        # 4.1.3 process upload
        # ret = self.post(f"vsms/admin/api/1.0/package/upload/{id}/process",json={},auth=self.access_token)
        headers = {}
        headers["Authorization"] = self.access_token
        url = self._make_url(f"vsms/admin/api/1.0/package/upload/{id}/process")
        with requests.post(url, headers=headers) as resp:
            logger.debug(f"status_code={resp.status_code} ({url})")
            logger.debug(f"{resp.text}")
            if resp.status_code in (200, 201, 202):
                # 'Ok'/'created'
                logger.info(f"### response: {resp.headers.get('Location')}")
                return resp.headers.get("Location")
            raise ValueError(f"{resp.status_code, url}")

    def vsms_admin__package__get_status(self, location: str):
        # 4.1.4 get processing status
        ret = self.get(f"{location}", auth=self.access_token)
        logger.debug(f"processing status:{ret}")
        return ret

    def vsms_admin__campaign__get(self, campaign_id: str):
        # 4.3.6 get campaign

        ret = self.get(
            f"vsms/admin/api/1.0/campaign/{campaign_id}", auth=self.access_token)
        return ret

    def vsms_admin__campaign__submit_for_approval(self, campaign_id: str, release_date: str):
        # 4.3.3 modify campaign for approval

        body = {}
        body["releaseDate"] = release_date
        # body["exprirationDate"] = expir
        logger.debug(f"campaigin submit for approval body {body}")

        ret = self.put(
            f"vsms/admin/api/1.0/campaign/{campaign_id}/submit", json=body, auth=self.access_token)
        return ret

    def vsms_admin__campaign__approval(self, campaign_id: str, release_date: str):
        # 4.3.4 modify campaign for approval

        body = {}
        body["releaseDate"] = release_date
        # body["exprirationDate"] =
        logger.debug(f"campaigin approval body {body}")

        ret = self.put(
            f"vsms/admin/api/1.0/campaign/{campaign_id}/approve", json=body, auth=self.access_token)
        return ret

    def vsms_admin__campaign__cancel(self, campaign_id: str):
        # 4.3.11 cancel campaign
        ret = self.put(
            f"vsms/admin/api/1.0/campaign/{campaign_id}/cancel", json={}, auth=self.access_token)
        return ret

    def help_desk__history_config__get(self, vin: str):
        ret = self.get(
            f"helpdesk/admin/api/1.0/vehicle/{vin}/vehicle_config", params={}, auth=self.access_token)
        return ret
