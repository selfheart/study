"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from string import Template
from typing import List, Dict, Union


Vars = Dict[str, Union[str, int]]


class Format:
    __variables: Vars = dict()
    string = ''
    __spaces = 0

    def __init__(self, variables: Vars) -> None:
        self.__variables.update(variables)

    def format(self, format_string: str) -> None:
        # expand variables
        s = Template(format_string).substitute(self.__variables)

        lines: List[str] = s.split('\n')
        # generate indent space string
        spaces = ''.join(['  ' for i in range(self.__spaces)])
        string = ''
        # add indent spaces to lines
        for line in lines:
            if line == '':
                string += '\n'
            else:
                string += spaces + line + '\n'

        # remove extra spaces and a line break
        self.string += string[0:-1]

    def indent(self) -> None:
        self.__spaces += 1

    def outdent(self) -> None:
        self.__spaces -= 1

    def set_variables(self, variables: Vars) -> None:
        self.__variables.update(variables)

    def add_variable(self, key: str, value: Union[str, int]) -> None:
        self.__variables[key] = value
