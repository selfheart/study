# url

http://192.168.8.191:32677/

# 参数约定
## vehicle
vin： vinUpdate20231008

primaryId：0

## attemptId
attemptId：一次升级需要保持唯一，当前建议直接使用compaign_id
## command 返回
"commandId":4,

"command":"StartCampaign", 
## action 返回
```json
{
    "actionId":"GetActionCycle",
    "value":0
},
{
    "actionId":"CarConfigInfoUploadNecessity",
    "value":0
},
{
    "actionId":"DispConsentInCarHMI",
    "value":0
}
```

## validity check 返回
"status":"Deployed",


# API_1-Notification请求消息格式

## CAMPAIGN STATUS通知(C_01_Req)
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0",
    "events": [
        {
          "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
          "eventsType": "rmState",
          "eventMode": "active",
          "rmStateExtras":{
              "rmVehState":"IDLE",
              "stateScope":"vehicle"
          },
          "currentTime": {
            "time": 1696928154,
            "clockSource": "default"
          },
          "status": "success",
          "attemptId": "123456789",
          "reportId": "1"
      }
    ]
  }
}
``` 

```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0",
    "events": [
        {
          "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
          "eventsType": "rmState",
          "eventMode": "active",
          "rmStateExtras":{
              "rmVehState":"IDLE",
              "stateScope":"vehicle"
          },
          "currentTime": {
            "time": 1696928154,
            "Time": 1696928154,
            "clockSource": "default"
          },
          "status": "success",
          "attemptId": "123456789",
          "reportId": "1"
      }
    ]
  }
}
``` 


## Download tracking notification (C_09_Req)
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0",
    "events": [
          {
              "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
              "eventsType": "tracking",
              "eventMode": "active",
              "currentTime": {
                  "time": 1696928154,
                  "clockSource": "default"
              },
              "status": "success",
              "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
              "reportId": "1",
              "trackingExtras": {
                  "fileId": "1",
                  "result": "success",
                  "transportType": "wifi",
                  "dlTime": {
                      "time": 1696928154,
                      "clockSource": "default"
                  },
                  "fileSize": 4
              }
          }
      ]
  }
}
``` 


## Download start notification (C_13_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "download",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "started",
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "downloadExtras": {
                    "downloadSize": 200,
                    "transportType": "wifi",
                    "bytesDownloaded": 100
                },
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 

## Download progress (C_15_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "download",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "inProgress",
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "downloadExtras": {
                    "downloadSize": 200,
                    "transportType": "wifi",
                    "bytesDownloaded": 100
                },
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 

## Download completion notification (C_16_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "download",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "success",
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "downloadExtras": {
                    "downloadSize": 200,
                    "transportType": "wifi",
                    "bytesDownloaded": 100,
                    "oemErrors": [
                        {
                            "phaseCode": 0,
                            "siteCode": 0,
                            "oemCode": 0,
                            "targetId": "1"
                        }
                    ]
                },
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 


## Software update completion confirmation result notification (C_25_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "ackinstall",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "accepted",
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1"
            }
        ]
    }
}
``` 

## Software update completion notification (C_35_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "updateComplete",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "success",
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1"
            }
        ]
    }
}
``` 

## Install Acceptance Notification (C_19_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "installation",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "accepted",
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 
## Installation progress (C_20_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "installation",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "inProgress",
                "installProgressExtras": {
                    "numEcus": 1,
                    "completeEcus": 0,
                    "ecuProgress": [
                        {
                            "totalBytes": 200,
                            "targetId": "1",
                            "bytesProcessed": 100
                        }
                    ]
                },
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 

## Notification of activation acceptance (C_22_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "activate",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "accepted",
                
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 

## Activation Progress Notification (C_23_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "activate",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "inProgress",
                "activateProgressExtras": {
                    "totalActivation": 1,
                    "completeActivation": 1
                },
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 
## Notification of activation completion confirmation result (C_24_Req)
- request
```json
{
    "signed": {
        "vin": "12345678901234567",
        "primaryId": "0",
        "events": [
            {
                "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
                "eventsType": "activate",
                "eventMode": "active",
                "currentTime": {
                    "time": 1696928154,
                    "clockSource": "default"
                },
                "status": "success",
                "activationExtras": {
                    "updateTimeUTS": 1696928154,
                    "updateStatus": "success",
                    "ecuInfo": [{
                        "targetId": "1",
                        "ecuSoftwareId": "1",
                        "updateStatus": "success",
                        "errorCodes": [
                        ],
                        "updateMethod": "ota"
                    }],
                    "oemErrors": []
                },
                "attemptId": "90469326c5cd45eb9a94d7c10c9744e9",
                "reportId": "1",
                "timeStarted": {
                    "time": 1696928154,
                    "clockSource": "default"
                }
            }
        ]
    }
}
``` 
## User Acceptance Result Notification (C_10_Req)
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0",
    "events": [
        {
        "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
        "eventsType": "notification",
        "eventMode": "active",
        "currentTime": {
          "time": "1696928154",
          "clockSource": "default"
        },
        "status": "accepted",
        "attemptId": "123456789",
        "reportId": "1"
      }
    ]
  }
}
``` 

## Download Acceptance Result Notification (C_12_Req)
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0",
    "events": [
        {
        "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
        "eventsType": "download",
        "eventMode": "active",
        "currentTime": {
          "time": "1696928154",
          "clockSource": "default"
        },
        "downloadExtras": {
            "downloadSize": 200,
            "transportType": "wifi",
            "bytesDownloaded": 100
        },
        "timeStarted": {
            "time": 1696928154,
            "clockSource": "default"
        },
        "status": "accepted",
        "attemptId": "123456789",
        "reportId": "1"
      }
    ]
  }
}
``` 


## Vehicle error notification (C_28_Req)
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0",
    "events": [{
        "eventsUuid": "90469326c5cd45eb9a94d7c10c9744e9",
        "eventsType": "error",
        "eventMode": "active",
        "currentTime": {
          "time": "1696928154",
          "clockSource": "default"
        },
        "errorExtras":[
          {
              "phaseCode": 0,
              "siteCode": 0,
              "oemCode": 1112,
              "targetId": "1"
          }
        ],
        "errorCodes":[
          {
              "code": 1112,
              "description": "error description"
          }
        ],
        "timeStarted": {
            "time": 1696928154,
            "clockSource": "default"
        },
        "status": "failure"
      }
    ]
  }
}
``` 








# API_2-OTA Action acquisition **OTA Action取得(C_03_Req)**
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0"
  },
}

```
- response
```json
{
    "signed": {
        "body":{
            "actions":[
                {
                    "actionId":"GetActionCycle",
                    "value":0
                },
                {
                    "actionId":"CarConfigInfoUploadNecessity",
                    "value":0
                },
                {
                    "actionId":"DispConsentInCarHMI",
                    "value":0
                }
            ]
        }
    }
}
```


# API_3-Vehicle configuration information digest **車両構成情報ダイジェスト(C_04_Req)**
- request
```json

```
- response
```json
{
  "signatures": [],
  "signed": {
    "body": {
      "targetsMetadata": [
        {
          "fileName": "mm",
          "metadata": {
            "signed": {
              "body": {
                "targets": [
                  {
                    "target": {
                      "fileName": "mm",
                      "length": 1,
                      "fileDownloadUrl": "http://192.168.248.191:32123/ota-24mm-dev/PUBLIC_APPLICATION/c48ef45d9689444d9ba3b5ef999e820c.zip?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Date=20231012T074621Z&X-Amz-SignedHeaders=host&X-Amz-Expires=86400&X-Amz-Credential=ARTIFACTORYMINIOACCESSKEY%2F20231012%2Fap-northeast-2%2Fs3%2Faws4_request&X-Amz-Signature=a187a8d3cd38a063d4afc51fd3839796d63ca9fed61dacecbc6a531c1f94397d"
                    },
                    "custom": {
                      "softwarePackageId": 1
                    }
                  }
                ]
              },
              "type": "targets"
            }
          }
        }
      ],
      "augmentedMetadata": {
        "signed": {
          "body": {
            "campaigns": [
              {
                "campaignId": 1
              },
              {
                "campaignId": 1
              },
              {
                "campaignId": 18
              }
            ],
            "status": "",
            "selectedCampaign": {
              "campaignId": 18
            }
          },
          "type": "augmented"
        }
      }
    }
  }
}
```

# API_4-Vehicle configuration information upload    **車両構成情報アップロード (C_05_Req)**
- request
```json
{
  "signed": {
    "vin": "TESTVIN1234567890",
    "ecuVersionManifests": [],
    "primaryId": "0",
    "clientDigest": "0",
    "augmentedManifest": {
      "timeStamp": "1697079831",
      "packageStorage": {
        "used": 100,
        "available": 100
      },
      "lastUpdateCampaignId": 0,
      "rxswin": [],
      "uploadReason": "test",
      "locale": "test",
      "preferredCampaignId": 18,
      "attemptId": "1697079831",
      "isNewAttempt": true
    }
  }
}
```
- response
```json
{
  "signatures": [],
  "signed": {
    "body": {
      "targetsMetadata": [
        {
          "fileName": "mm",
          "metadata": {
            "signed": {
              "body": {
                "targets": [
                  {
                    "target": {
                      "fileName": "mm",
                      "length": 1,
                      "fileDownloadUrl": "http://192.168.248.191:32123/ota-24mm-dev/PUBLIC_APPLICATION/c48ef45d9689444d9ba3b5ef999e820c.zip?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Date=20231012T074621Z&X-Amz-SignedHeaders=host&X-Amz-Expires=86400&X-Amz-Credential=ARTIFACTORYMINIOACCESSKEY%2F20231012%2Fap-northeast-2%2Fs3%2Faws4_request&X-Amz-Signature=a187a8d3cd38a063d4afc51fd3839796d63ca9fed61dacecbc6a531c1f94397d"
                    },
                    "custom": {
                      "softwarePackageId": 1
                    }
                  }
                ]
              },
              "type": "targets"
            }
          }
        }
      ],
      "augmentedMetadata": {
        "signed": {
          "body": {
            "campaigns": [
              {
                "campaignId": 1
              },
              {
                "campaignId": 1
              },
              {
                "campaignId": 18
              }
            ],
            "status": "",
            "selectedCampaign": {
              "campaignId": 18
            }
          },
          "type": "augmented"
        }
      }
    }
  }
}
```

# API_5-Campaign validity check **キャンペーン有効性確認(C_18_Req)**
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0",
    "campaignId": 1
  }
}
```
- response
```json
{
    "signed": {
        "body":{
            "vin":"12345678901234567",
            "status":"Deployed",
            "campaignId":0
        }
    }
}
```
# API_6-OTA Command acquisition **OTAコマンド取得(C_36_Req)**
- request
```json
{
  "signed": {
    "vin": "12345678901234567",
    "primaryId": "0"
  },
}
```
- response
```json
{
    "signed": {
        "body":{
            "vin":"12345678901234567",
            "commandId":4,
            "command":"StartCampaign", 
            "campaignId":0
        }
    }
}
```
# API_7-Vehicle Log Upload URL Acquisition  **車両ログアップロードURL取得(C_37_Req)**
- request
```json

```
- response
```json

```
# API_8-Vehicle Log Upload Completion Notice **車両ログアップロード完了通知(C_39_Req)**
- request
```json

```
- response
```json

```

# CDN
## Repro policy META Data acquisition (C_06_Req)
## Download META Data acquisition (C_07_Req)
## HMI Data acquisition (C_08_Req)
## VehiclePackage DL (C_11_Req)
## SoftwarePackage DL (C_14_Req)
