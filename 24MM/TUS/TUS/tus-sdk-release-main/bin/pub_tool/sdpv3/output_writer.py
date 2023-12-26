"""
TMC CONFIDENTIAL
$TUSLibId$
Copyright (C) 2022 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""


class OutputWriter():
    level_space = "  "  # two spaces
    string_list_field = [
        "changeRequests",
        "rxswins",
        "rxswin",
        "previousRxswins",
        "dependentSystems",
        "vehicleTypeIds",
        "start",
        "result",
        "reportedRxswins",
        "equippedSystems"
    ]
    table_field = [
        "subsystemConfigurations",
        "vehicleTypes",
        "vehicleTypeCheckFlagsList",
        "softwareInfos",
        "campaignInfos",
        "subTargetReproTypes",
        "campaignList",
        "configurations"
    ]

    def write_line(self, level: int, format: str):
        prefix = ""
        for i in range(0, level):
            prefix += self.level_space
        print(prefix + format)

    def write_table(self, header: list, data: list, level: int = 0):
        separator = ["-" * length for length in map(len, header)]
        body = []
        body.append(header)
        body.append(separator)
        for i in range(len(data)):
            row = []
            for key in header:
                try:
                    row.append(data[i][key])
                except KeyError:
                    row.append("")  # output blank if key does not exist
            body.append(row)
        lengths = []
        for row in body:
            row = list(map(str, row))
            lengths.append(list(map(len, row)))
        space = []
        for i in range(len(header)):
            space.append(max([length[i] for length in lengths]))

        s = "".join(["{:<{}}\t  " for _ in range(len(header))])
        for i in range(len(body)):
            args = []
            for j in range(len(header)):
                args.append(str(body[i][j]))
                args.append(space[j])
            self.write_line(level, s.format(*args))

    def write(self, body: dict, level: int = 0):
        max_length = 0
        length_list = [len(k) for k, v in body.items() if type(v) != list and type(v) != dict]
        if len(length_list) > 0:
            max_length = max(length_list)
        for k, v in body.items():
            space = 8 * (max_length // 8 + 1)
            if type(v) != list and type(v) != dict:
                self.write_line(level, "{key:<{space}}  {value}".format(key=k + ":", space=space, value=v))
            elif type(v) == dict:
                self.write_line(level, "{}:".format(k))
                self.write(v, level + 1)
            elif type(v) == list and k in self.string_list_field:
                joined_v = ", ".join(map(str, v)) if len(v) > 0 else "None"
                self.write_line(level, "{key:<{space}}  {value}".format(key=k + ":", space=space, value=joined_v))
            elif type(v) == list:
                if len(v) == 0:
                    self.write_line(level, "{key:<{space}}  None".format(key=k + ":", space=space))
                elif k in self.table_field:
                    self.write_line(level, "{}:".format(k))
                    # print table
                    keys = []
                    for component in v:
                        keys += component.keys()
                    unique_key = list(dict.fromkeys(keys))  # eliminate duplicated keys
                    self.write_table(unique_key, v, level + 1)
                else:
                    self.write_line(level, "{}:".format(k))
                    if len(v) == 1:
                        self.write(v[0], level + 1)
                    else:
                        for index, component in enumerate(v):
                            self.write_line(level + 1, "index:\t{}".format(index))
                            self.write(component, level + 2)

    def output_table(self, body: list):
        if len(body) == 0:
            self.write_line(0, "No data found.")
            return
        keys = []
        for component in body:
            keys += component.keys()
        unique_key = list(dict.fromkeys(keys))  # eliminate duplicated keys
        self.write_table(unique_key, body)
        return
