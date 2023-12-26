"""
TMC CONFIDENTIAL
$JITDFLibId$
Copyright (C) 2021 TOYOTA MOTOR CORPORATION
All Rights Reserved.
"""
from google.protobuf.compiler.plugin_pb2 import CodeGeneratorRequest, CodeGeneratorResponse
from google.protobuf.descriptor import FileDescriptor
from google.protobuf.descriptor_pb2 import FileDescriptorProto
from google.protobuf.descriptor_pool import DescriptorPool
from sys import stdin, stdout
from pathlib import PurePath
from . import generator


__version__ = '0.0.1'


def main() -> None:
    buffer = stdin.buffer.read()
    request = CodeGeneratorRequest()
    # read parsed .proto file
    request.ParseFromString(buffer)

    pool = DescriptorPool()
    file_desc_proto: FileDescriptorProto
    for file_desc_proto in request.proto_file:
        # add FileDescriptorProto to DescriptorPool
        pool.Add(file_desc_proto)

    response = CodeGeneratorResponse()
    for filename in request.file_to_generate:
        # get FileDescriptor from a file name
        file_descriptor: FileDescriptor = pool.FindFileByName(filename)
        # add a file which represents a single generated file
        file: CodeGeneratorResponse.File = response.file.add()
        # set a generated file name
        file.name = PurePath(filename).stem + '_pb.lua'
        # generate code
        file.content = generator.Generator(
            file_descriptor, request.parameter).generate_code()

    # write <file_name>_pb.lua files
    stdout.buffer.write(response.SerializeToString())
